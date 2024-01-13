#ifndef __SYS_H__
#define __SYS_H__

extern void DELAY(int);
extern void ebreak(void);
extern void monitor(void);
extern int memfault;
extern void setmepc(uint32_t);
extern void setmtie(void);
extern void clrmtie(void);

#endif /* __SYS_H__ */
