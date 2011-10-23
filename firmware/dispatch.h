// dispatch.h

void initanim(void);
void initdisplay(uint8_t inverted);
void drawdisplay(uint8_t inverted);
void step(void);

void initanim_rat(void);
void initanim_int(void);
void initanim_sev(void);
void initanim_xda(void);
void initanim_death(void);
void initanim_mar(void);

void initdisplay_rat(uint8_t);
void initdisplay_int(uint8_t);
void initdisplay_sev(uint8_t);
void initdisplay_xda(uint8_t);
void initdisplay_death(uint8_t);
void initdisplay_mar(uint8_t);

void drawdisplay_rat(uint8_t);
void drawdisplay_int(uint8_t);
void drawdisplay_sev(uint8_t);
void drawdisplay_xda(uint8_t);
void drawdisplay_death(uint8_t);
void drawdisplay_mar(uint8_t);

void step_rat(void);
void step_int(void);
void step_sev(void);
void step_xda(void);
void step_death(void);

void initanim_abo(void);

void initanim_deathcfg(void);

void initanim_GPS(void);

uint16_t crand(uint8_t type);
void init_crand(void);
