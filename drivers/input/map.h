#ifndef __MAP_H__

#define __MAP_H__

enum map_mode{
	NORMAL=0,							// normal running mode
	SETTING,							// call to setting gamemap layout
};

extern void process_sync(struct input_dev *dev);
extern void process_map(struct input_dev *dev,int type,int code, int value);
extern int keymap_input_init(void);
extern void keymap_input_remove(void);
#endif
