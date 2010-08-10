// dispatch.h

void initanim(void);
void initdisplay(uint8_t inverted);
void drawdisplay(uint8_t inverted);
void step(void);

void initanim_rat(void);
void initanim_int(void);
void initanim_sev(void);
void initanim_xda(void);

void initdisplay_rat(uint8_t);
void initdisplay_int(uint8_t);
void initdisplay_sev(uint8_t);
void initdisplay_xda(uint8_t);

void drawdisplay_rat(uint8_t);
void drawdisplay_int(uint8_t);
void drawdisplay_sev(uint8_t);
void drawdisplay_xda(uint8_t);

void step_rat(void);
void step_int(void);
void step_sev(void);
void step_xda(void);
