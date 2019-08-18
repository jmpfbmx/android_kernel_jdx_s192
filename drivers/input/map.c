#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/wait.h>

#include <linux/suspend.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/list.h>
#include <linux/input/mt.h>  

#include "map.h"

struct key_mapcode{
	int id;
	int scan_code;
	int x;
	int y;
	int r;
	int mode;
	int level;
	struct key_mapcode *next;

};

typedef struct map_data
{
	struct input_dev *mapinput;
	struct key_mapcode* map_head;

}pmap_data;

enum{
	
	VIEW_MODE=1,
	STICK_MODE=2,
	
};
#if 1
#define MAPDBG(v...)
#else
#define MAPDBG(v...) printk(v)

#endif

static int debug_enable = 0;

static ssize_t debug_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", debug_enable);
}

static ssize_t debug_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long data;

	if (strict_strtoul(buf, 10, &data))
		return -EINVAL;

	if ((data == 0) || (data == 1))
	{
		debug_enable = data;
	}

	return count;
}

static DEVICE_ATTR(debug, 0660, debug_enable_show, debug_enable_store);

#define SCREEN_MAX_X		1920
#define SCREEN_MAX_Y		1200
#define LEFT_JOY		13
#define RIGHT_JOY		12
#define HOT_KEY			0x58
#define MAX_POINT		20

#define JOY_CENTER			128
#define JOY_RADIUS			128
#define IGNORE_VIEW_COUNTER		10

#define KEYMAP_NAME			"hwpdata"
#define KEYMAP				0xA1
#define MAP_IOCTL_WRITE                 _IOW(KEYMAP, 0x01, char[408])
#define MAP_IOCTL_MODE                 _IOW(KEYMAP, 0x01, char)

#define KEY_NUM	20

#define FIX_X(v)  	(SCREEN_MAX_X-v)
#define FIX_Y(v)	(v)	


#define SWAP(x,y)	\
{					\
	int temp=0;		\
	temp=x;			\
	x=FIX_X(y);		\
	y=FIX_Y(temp);	\
}					

static int LEFT_XY[2]={JOY_CENTER,JOY_CENTER};
static int RIGHT_XY[2]={JOY_CENTER,JOY_CENTER};
static int PRE_L_R_XY[4]={JOY_CENTER,JOY_CENTER,JOY_CENTER,JOY_CENTER};
static struct key_mapcode key_mapvalue[KEY_NUM]={{0}};

static pmap_data * private_data;
static int map_flag,viewx,viewy,origin_viewx,origin_viewy;

static int lstick_x=128,lstick_y=128,rstick_x=128,rstick_y=128;

static struct input_dev *key_input;
extern int early_suspend_flag;


char setting_mode=NORMAL;


static int mKeyCode_XY[][6] = {
	{103, -1, -1, 0, 0, 0},//ÉÏ  0x67
	{108, -1, -1, 0, 0, 0},//ÏÂ	0x6c
	{105, -1, -1, 0, 0, 0},//×ó	0x69
	{106, -1, -1, 0, 0, 0},//ÓÒ	0x6a
	{308, -1, -1, 0, 0, 0},//X		0x134
	{307, -1, -1, 0, 0, 0},//Y		0x133
	{304, -1, -1, 0, 0, 0},//A        0x130
	{305, -1, -1, 0, 0, 0},//B		0x131
	
	{0x136, -1, -1, 0, 0, 0},//L1
	{0x138, -1, -1, 0, 0, 0},//L2
	{0x137, -1, -1, 0, 0, 0},//R1
	{0x139, -1, -1, 0, 0, 0},//R2	
	
	{0x13a, -1, -1, 0, 0, 0},//select
	{0x13b, -1, -1, 0, 0, 0},//start
	
	{LEFT_JOY, -1, -1, 0, 0, 0},//×óÒ¡¸Ë
	{RIGHT_JOY, -1, -1, 0, 0, 0},//ÓÒÒ¡¸Ë
	{0, -1, -1, 0, 0, 0},//ÓÒÒ¡¸Ë
	
	{0x13d, -1, -1, 0, 0, 0},//L3
	{0x13e, -1, -1, 0, 0, 0},//R3

};



static int keymap_open(struct inode *inode, struct file *file)
{

	return 0;
}

static int keymap_release(struct inode *inode, struct file *file)
{

	return 0;
}

static long keymap_ioctl(struct file *file, unsigned int cmd,  unsigned long arg)
{
	void __user *argp = (void __user *) arg;
	int code_size=0;
	int id=0;
	
	switch (cmd) {
	case MAP_IOCTL_WRITE:
		if (copy_from_user(&mKeyCode_XY, argp, sizeof(mKeyCode_XY)))
			return -EFAULT;
		break;
	case MAP_IOCTL_MODE:
		if (copy_from_user(&setting_mode, argp, sizeof(setting_mode)))
				return -EFAULT;

	default:
		break;
	}
	code_size=sizeof(mKeyCode_XY)/24;  //	( sizeof() /6) / 4
//	MAPDBG("get code size =%d\n",code_size);
/*
	if (mKeyCode_XY[5][1]>0 && mKeyCode_XY[5][2]>0)
		map_flag=true;
	else
		map_flag=false;
*/
	map_flag=false;

	for (id=0;id<code_size;id++){
		key_mapvalue[id].id=id;
		key_mapvalue[id].scan_code=mKeyCode_XY[id][0];
		key_mapvalue[id].x=mKeyCode_XY[id][1];
		key_mapvalue[id].y=mKeyCode_XY[id][2];		
		key_mapvalue[id].r=mKeyCode_XY[id][3];
		key_mapvalue[id].mode=mKeyCode_XY[id][4];
		key_mapvalue[id].level=mKeyCode_XY[id][5];

		if (key_mapvalue[id].x >0 ||key_mapvalue[id].y > 0)
			map_flag=true;
		
		//SWAP(key_mapvalue[id].x,key_mapvalue[id].y);

		if (key_mapvalue[id].scan_code==RIGHT_JOY){
			origin_viewx=(key_mapvalue[id].x );
			origin_viewy=(key_mapvalue[id].y );
			viewx=origin_viewx;
			viewy=origin_viewy;	
		}
		
		MAPDBG("%d, %d,%d,%d,%d,%d,%d \n",id,key_mapvalue[id].scan_code,key_mapvalue[id].x,key_mapvalue[id].y,key_mapvalue[id].r,key_mapvalue[id].mode,key_mapvalue[id].level);
	}


	return 0;
}


static const struct file_operations keymap_fops = {
	.owner = THIS_MODULE,
	.open = keymap_open,
	.release = keymap_release,
	.unlocked_ioctl = keymap_ioctl,
};


static struct miscdevice keymap_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "key_daemon",
	.fops = &keymap_fops,
};


static int find_id(int scancode)
{
	int i=0;
	for (i=0;i<KEY_NUM;i++){
		if (scancode==key_mapvalue[i].scan_code)
			return i;
	}
	return -1;

}

void send_keypress (int x, int y, int id)
{
	
	input_report_abs(private_data->mapinput,  ABS_MT_SLOT, id);			
	input_report_abs(private_data->mapinput,  ABS_MT_TRACKING_ID, id);	
	input_report_abs(private_data->mapinput,  ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(private_data->mapinput,  ABS_MT_POSITION_X, x);
	input_report_abs(private_data->mapinput,  ABS_MT_POSITION_Y, y);
	
}

 void send_keyrelease (int id)
 {
	input_report_abs(private_data->mapinput,  ABS_MT_SLOT, id);		
	input_report_abs(private_data->mapinput,  ABS_MT_TRACKING_ID, -1);	
	
}

static int view_step_array[6]={0,8,6,3,2,1};  //   default:   3*10= 30 ms

#define STEP1    15    //  15*30=450ms
#define STEP2    20   // 6*30=180ms
#define STEP3    30  
#define STEP4    40   
#define STEP5    50 
#define STEP6    60   

static int view_turnaround=0;
static void handle_viewmap(int code,int value,int id,int level)
{
	static unsigned int running_count=0,space_count=0;								// 10ms duration
	static int x=0,y=0;
	int view_step,abs_x,abs_y;
	int step=1;
	int leverage=view_step_array[level];
	static unsigned int pre_value=0;
	
	if (ABS_Z==code){
		x=value-JOY_CENTER;
		x=x;
	}else if (ABS_RZ==code){
		y=value-JOY_CENTER;
		y=y;
	}
	

	abs_x=abs(x);
	abs_y=abs(y);



	if (x==0 && y==0){
		space_count=0;
		return;
	}

	if (abs_y<23)              // fix to be horizon
		y=0;
	
	running_count++;

	if (running_count% leverage !=0)
		return;
	running_count=0;
	space_count++;
	
	if (space_count<STEP1){    //	2 *30 = 60ms
		view_step=step;
	}else if (space_count<STEP2){
		view_step=2*step;

	}else if (space_count<STEP3){
		view_step=3*step;

	}else if (space_count<STEP4){
		view_step=4*step;

	}else if (space_count<STEP5){
		view_step=5*step;

	}else if (space_count<STEP6){
		view_step=6*step;

	}else{
		view_step=7*step;
		space_count--;     // keep value here
	}

	if (abs_x < pre_value ||  abs_x <96){
		space_count=STEP1;
		view_step=2*step;
		
	}
	
	pre_value=abs_x;

	
	MAPDBG(" : v=%d x,y[%d:%d] \n",view_step,x,y);

	if (y==0){

		if (x < 0) 
			view_step=(-1)*view_step;
		
		viewx=viewx+view_step;
		viewy=viewy;
		//MAPDBG(" :::::::: step=%d, viewx[%d:%d]\n ",view_step,viewx,viewy);

	}else if (x==0){
		if (view_step>3)		//  points less than 3 points in Y axis
			view_step=3;
		if (y < 0 )
			view_step=(-1)*view_step;
		
		viewx=viewx;
		viewy=viewy+view_step;
		

	}else{
		if (view_step>3)		//  points less than 3 points in lean axis
			view_step=2;

		if (x>0 && y<0 ){			// zero 1 for touchscreen

	
				viewx+=view_step;
				viewy+=(-1)*view_step;
			
		
		}else if (x>0 && y>0 ){		// zero 2

	
				viewx+=view_step;
				viewy+=view_step;
			
		

		}else if (x < 0 && y>0){		//zero 3

	
				viewx+=(-1)*view_step;
				viewy+=view_step;

			

		}else if (x<0 && y<0){			// zero 4


		
				viewx+=(-1)*view_step;
				viewy+=(-1)*view_step;

		
		}	
				
	}


	if (viewy>SCREEN_MAX_Y ||viewy<0){     // still be vertical screen, the right top is zero
		viewx=origin_viewx;
		viewy=origin_viewy;	
		view_turnaround=true;
	}	
	if (viewx < (SCREEN_MAX_X/2 + 2)|| viewx > SCREEN_MAX_X){
		viewx=origin_viewx;
		viewy=origin_viewy;	
		view_turnaround=true;
	}
	
	MAPDBG(" :viewstep=%d, viewx[%d:%d]\n ",view_step,viewx,viewy);


	return ;


}

extern void get_suspend_flag(void);
extern void put_suspend_flag(void);

static void handle_stickmap(int code,int value,int id)
{
	int step=0;

	step= (key_mapvalue[id].r *(value-JOY_CENTER))/JOY_RADIUS;	 //  r * (x-o)/len

	if(debug_enable)
		printk("%s  %s  %d , key_mapvalue[%d].r = %d , value= %d , step = %d\n",__FILE__,__func__,__LINE__,id,key_mapvalue[id].r,value,step);

	if (code==ABS_X ){			  // if axis Y changes, then is the ts X changes
		
		lstick_x= key_mapvalue[id].x+ step;
		
	}else if (code==ABS_Y){	
		
		lstick_y=key_mapvalue[id].y+ step;
		
	}else if (code==ABS_Z){	
	
		rstick_x=key_mapvalue[id].x+ step;
		
	}else if (code==ABS_RZ){
		
		rstick_y=key_mapvalue[id].y+ step;


	}
	if(debug_enable)
		printk("%s  %s  %d , lstick_x = %d , lstick_y = %d , rstick_x =%d , rstick_y = %d\n",__FILE__,__func__,__LINE__,lstick_x,lstick_y,rstick_x,rstick_y);

}


void process_sync(struct input_dev *dev)
{

	if (setting_mode == SETTING && key_input) {
		dev = key_input;
	}

   input_sync(dev);
   return;
}

static void emulate_ts(void)
{
	struct key_mapcode *temp;
	
	temp=private_data->map_head->next;
	while(temp!=NULL){
		if (temp->scan_code==LEFT_JOY){
		
			send_keypress(lstick_x,lstick_y,temp->id);
			

		}else if (temp->scan_code==RIGHT_JOY){
			if (temp->mode==STICK_MODE)
				send_keypress(rstick_x,rstick_y,temp->id);
			else if (temp->mode==VIEW_MODE)
				send_keypress(viewx,viewy,temp->id);

		}else{
			send_keypress(temp->x,temp->y,temp->id);
		}
		temp=temp->next;
	}
	process_sync(private_data->mapinput);
}


static void insert_list(int id)
{
	struct key_mapcode *temp;
	temp=private_data->map_head;
	while(temp->next!=NULL){
		if (temp->next->id == id){
			//MAPDBG("find the same list to insert, to return \n");
			return;
		}
		temp=temp->next;
	}

	temp->next= &key_mapvalue[id];
	key_mapvalue[id].next=NULL;

	return;

}

static void delete_list(int id)
{

	struct key_mapcode *temp,*del_node;

	temp=private_data->map_head;
	while(temp->next!=NULL){
		if (temp->next->id == id){
			del_node=temp->next;
			temp->next=temp->next->next;
			del_node->next=NULL;
			return;
		}
		temp=temp->next;
	}
//	MAPDBG("Error: find wrong list to delete id=%d, code=%d\n",id,key_mapvalue[id].scan_code);

	return;
}



struct union_value{
	int id;
	int real_value;
}fake_value[]={
	{0,0},
	{KEY_F13,103 },
	{KEY_F14,108 },
	{KEY_F15,105 },
	{KEY_F16,106 },
	
	{KEY_F17,304 },
	{KEY_F18,305 },
	{KEY_F19,308 },
	{KEY_F20,307 },
	
	{KEY_F21, 0x136 },
	{KEY_F22,0x137 },
	{KEY_F23,0x138},
	{KEY_F24,0x139 },
	{BTN_BASE,0x13e },
	{BTN_BASE2,0x13a },
	{BTN_BASE3,0x13b },
//	{BTN_BASE4,0x13e },        // for joystick
//	{BTN_BASE5,0x13f },
	{BTN_BASE6,0x13d },


};

int get_id(int scancode)
{
	int size=ARRAY_SIZE(fake_value);
	int i=0;
	for (i=0;i<size;i++){
		if (scancode == fake_value[i].real_value)
			return fake_value[i].id;
	}
	return -1;

}

void set_kedev(struct input_dev *dev)
{
	key_input = dev;

	return;
}


static unsigned int center_ignore_cnt=0;
#define CENTER_IGNORE_CNT	15			// 200ms

static int leftjoy[2]={0x80,0x80};
static int rightjoy[2]={0x80,0x80};

static unsigned int left_flag_1,left_flag_0,right_flag_1,right_flag_0;
static int do_compare(int a,int b)
{
	int result;
	int ret = 0;

	result = (a > b) ? (a - b) : (b-a);

	if(result >= 3)
		ret = 1;
	else
		ret = 0;

	return ret;
}


 void process_map(struct input_dev *dev,int type,int code, int value)
{
	int id=-1;
	int press=value;
	int scan_code=code;
	static int ignore_viewcnt=0;
	int need_to_report = 0;
	get_suspend_flag();
	if (early_suspend_flag == 1 && code != KEY_POWER){
		put_suspend_flag();
		return;
	}
	put_suspend_flag();
	if ((KEY_HOME==code || KEY_BACK==code || HOT_KEY ==code  || KEY_POWER ==code \
		|| KEY_VOLUMEUP==code || KEY_VOLUMEDOWN ==code)&& EV_KEY==type){
		input_report_key(dev, code, value);
		
		return;
	}

	if(debug_enable)
		printk("%s  %s  %d , setting mode = %d\n",__FILE__,__func__,__LINE__,setting_mode);

	if (setting_mode == SETTING && key_input) {                  // for gamemap setting menu
		if (type == EV_KEY)
			id = get_id(code);
		else{
			if (type == EV_ABS && (code == ABS_X ||code ==ABS_Y)){
				if (code == ABS_X)
					leftjoy[0]=value;
				else
					leftjoy[1]=value;
				if (leftjoy[0]==0x80 && leftjoy[1]==0x80){
					id =BTN_BASE4;
					value =0;
					
				}else{
					id =BTN_BASE4;
					value=1;
				
				}
			}else if (type == EV_ABS && (code == ABS_Z ||code ==ABS_RZ)){
				if (code == ABS_Z)
					rightjoy[0]=value;
				else
					rightjoy[1]=value;
				if (rightjoy[0]==0x80 && rightjoy[1]==0x80){
					id =BTN_BASE5;
					value =0;
				
				}else{
					id =BTN_BASE5;
					value=1;
					
				}
			}

			if (value == 1 && id ==BTN_BASE4){
				left_flag_1 ++;
				left_flag_0 =0;
			}else if (value ==0 && id ==BTN_BASE4){
				left_flag_1 =0;
				left_flag_0 ++;
			}
			if (value == 1 && id ==BTN_BASE5){
				right_flag_1 ++;
				right_flag_0 =0;
			}else if (value ==0 && id ==BTN_BASE5){
				right_flag_1 =0;
				right_flag_0 ++;
			}
			if (id == BTN_BASE4){
				if (left_flag_0 > 1 || left_flag_1 > 1 )
					return;

			}else{
				if (right_flag_0 > 1 || right_flag_1 > 1 )
					return;
			}
	

		}		
		input_report_key(key_input,id,value);
		if (id==BTN_BASE4 || id ==BTN_BASE5)
			  input_sync(key_input);
	//	printk("in gamemap setting mode: scan_code=%d,keycode=%d ,value=%d\n",code,id,value);
		return;		
	}
	
	
	if (EV_ABS==type){
		if (ABS_X==code){
			scan_code=LEFT_JOY;
			LEFT_XY[0]=value;

			if(do_compare(LEFT_XY[0],PRE_L_R_XY[0]))
			{
				PRE_L_R_XY[0] = LEFT_XY[0];
				need_to_report = 1;
			}
		}else if (ABS_Y==code){
			scan_code=LEFT_JOY;
			LEFT_XY[1]=value;

			if(do_compare(LEFT_XY[1],PRE_L_R_XY[1]))
			{
				PRE_L_R_XY[1] = LEFT_XY[1];
				need_to_report = 1;
			}
		}else if (ABS_Z==code){
			scan_code=RIGHT_JOY;
			RIGHT_XY[0]=value;

			if(do_compare(RIGHT_XY[0],PRE_L_R_XY[2]))
			{
				PRE_L_R_XY[2] = RIGHT_XY[0];
				need_to_report = 1;
			}
	
		}else if (ABS_RZ==code){
			scan_code=RIGHT_JOY;
			RIGHT_XY[1]=value;

			if(do_compare(RIGHT_XY[1],PRE_L_R_XY[3]))
			{
				PRE_L_R_XY[3] = RIGHT_XY[1];
				need_to_report = 1;
			}
		}
		if(debug_enable)
			printk("%s  %s  %d , code = 0x%x , value = %d\n",__FILE__,__func__,__LINE__,code,value);
	}
	
	id=find_id(scan_code);
	if (id<0){
		MAPDBG(" error: find code error, return code=0x%x \n",code);
	//	return;
	}

	if ((id <0) ||( id > 0 &&(key_mapvalue[id].x <0 || key_mapvalue[id].y <0))){

		if(debug_enable)
			printk("%s  %s  %d , id = 0x%d 111111\n",__FILE__,__func__,__LINE__,id);
		if (EV_KEY==type){
		
			if (code==BTN_TL2 ){
				input_report_abs(dev,0xa,value);
				input_sync(dev);
				input_report_key(dev, code, value);
			}else if (code == BTN_TR2){
		
				input_report_abs(dev,0x9,value);
				input_sync(dev);
				input_report_key(dev, code, value);
			}else if(code == KEY_LEFT){
				input_report_abs(dev,ABS_HAT0X,value==1?-1:0);
			}else if(code == KEY_RIGHT){
				input_report_abs(dev,ABS_HAT0X,value==1?1:0);
			}else if(code == KEY_UP){
				input_report_abs(dev,ABS_HAT0Y,value==1?-1:0);
			}else if(code == KEY_DOWN){
				input_report_abs(dev,ABS_HAT0Y,value==1?1:0);	
			}else{
				input_report_key(dev, code, value);
			}
		}else if(EV_ABS==type)
			if(need_to_report)
				input_report_abs(dev, code, value);

	}else{

		if(debug_enable)
			printk("%s  %s  %d , id = 0x%d 22222\n",__FILE__,__func__,__LINE__,id);
		if (scan_code==LEFT_JOY){
			if (LEFT_XY[0]==JOY_CENTER && LEFT_XY[1]==JOY_CENTER){
				center_ignore_cnt++;
				if (center_ignore_cnt>CENTER_IGNORE_CNT){
					center_ignore_cnt=CENTER_IGNORE_CNT+1;
					press=false;
				}else
					press=true;
			}else {
				center_ignore_cnt=0;
				press=true;			
			}	
			handle_stickmap(code,value,id);
		}else if (scan_code==RIGHT_JOY){
			if (RIGHT_XY[0]==JOY_CENTER && RIGHT_XY[1]==JOY_CENTER){
				viewx=origin_viewx;
				viewy=origin_viewy;
				press=false;
			}else 
				press=true;
			if (key_mapvalue[id].mode==VIEW_MODE){
				handle_viewmap(code,value,id,key_mapvalue[id].level);
			}else{
				handle_stickmap(code,value,id);
			}
		}
				
		if (press==true){
			if (view_turnaround==true){ 		// to preven pointer too fast to pck up and then down again 
				if (ignore_viewcnt==0){
					delete_list(id);
					send_keyrelease(id);
				}
				ignore_viewcnt++;
				process_sync(private_data->mapinput);
				if (ignore_viewcnt<IGNORE_VIEW_COUNTER){

					goto ignore_view;
				}else{
					ignore_viewcnt=0;
					view_turnaround=false;

				}
			}
		
			insert_list(id);

		}else{
			
				delete_list(id);
				send_keyrelease(id);
				process_sync(private_data->mapinput);

			
		}
ignore_view:		
		emulate_ts();
	}

}

void keymap_input_remove(void)
{
	map_flag=0;
	misc_deregister(&keymap_device);
	if (private_data){
		kfree(private_data->mapinput);
		private_data->mapinput=NULL;
		
		kfree(private_data->map_head);
		private_data->map_head=NULL;
		
		kfree(private_data);
		private_data=NULL;
	}
	return;
}

int keymap_input_init(void)
{
	int err=0;
	struct input_dev *input_dev;
	private_data = kzalloc(sizeof(pmap_data), GFP_KERNEL);
	if (!private_data)
	{
		MAPDBG(KERN_ERR "Fail to kzalloc private_data\n");
		err= -ENOMEM;
		goto err_exit;
	}

	private_data->map_head=(struct key_mapcode*)kzalloc(sizeof(struct key_mapcode),GFP_KERNEL);
	if (!private_data->map_head)
	{
		MAPDBG(KERN_ERR " Fail to allocate map head\n");
		err = -ENOMEM;
		goto err_head_mem;
	}		

	private_data->mapinput = input_allocate_device();
	if (!private_data->mapinput)
	{
		MAPDBG(KERN_ERR "Fail to allocate mapinput device\n");
		err = -ENOMEM;
		goto err_input_mem;
	}
	input_dev=private_data->mapinput;

	input_mt_init_slots(input_dev, MAX_POINT,0);

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID, input_dev->absbit);


	input_set_abs_params(input_dev,ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,ABS_MT_POSITION_Y, 0,SCREEN_MAX_Y, 0, 0);

	input_set_abs_params(input_dev,ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);


	input_dev->name 	= KEYMAP_NAME;		
	err = input_register_device(input_dev);
	if (err) {
		goto error;
	}

	err = device_create_file(&input_dev->dev,&dev_attr_debug);
	if(err < 0) {
		printk(KERN_ALERT"Failed to create attribute debug.");

	}
	misc_register(&keymap_device);

	return 0;
		
error:
	kfree(private_data->mapinput);
err_input_mem:
	kfree(private_data->map_head);

err_head_mem:
	kfree(private_data);
err_exit:
	
	return err;
	}


EXPORT_SYMBOL(process_sync);
EXPORT_SYMBOL(process_map);





MODULE_LICENSE("GPL");
MODULE_AUTHOR("jxd");


