#include <Wire.h>

#define TOU_THRESH	0x0F
#define	REL_THRESH	0x0A

#define MHD_R	0x2B
#define NHD_R	0x2C
#define	NCL_R 	0x2D
#define	FDL_R	0x2E
#define	MHD_F	0x2F
#define	NHD_F	0x30
#define	NCL_F	0x31
#define	FDL_F	0x32
#define	ELE0_T	0x41
#define	ELE0_R	0x42
#define	ELE1_T	0x43
#define	ELE1_R	0x44
#define	ELE2_T	0x45
#define	ELE2_R	0x46
#define	ELE3_T	0x47
#define	ELE3_R	0x48
#define	ELE4_T	0x49
#define	ELE4_R	0x4A
#define	ELE5_T	0x4B
#define	ELE5_R	0x4C
#define	ELE6_T	0x4D
#define	ELE6_R	0x4E
#define	ELE7_T	0x4F
#define	ELE7_R	0x50
#define	ELE8_T	0x51
#define	ELE8_R	0x52
#define	ELE9_T	0x53
#define	ELE9_R	0x54
#define	ELE10_T	0x55
#define	ELE10_R	0x56
#define	ELE11_T	0x57
#define	ELE11_R	0x58
#define	FIL_CFG	0x5D
#define	ELE_CFG	0x5E

#define MPR121_ADDR 0x5A

#define NONE 0
#define PRESSED 1
#define HOLD 2
#define IDLE 3

#define MULTICLK_ON 0
#define MULTICLK_OFF 1

enum state{idle,press,holding,mul_click,released};

#define ELO_T 1
class MPR121{
    public:
        state stt;
        uint8_t cnt;
        MPR121();
        void InitConfig();
        char getKey();
        uint8_t GetState(uint8_t mode);
        void UpdateTouchreg();
        void UpdateState(state newstt);
    private:
    char _Keych[12] = {'*','7','4','1','0','8','5','2','#','9','6','3'};
    void WriteReg(uint8_t reg_addr,uint8_t val);
    uint16_t ReadReg(uint8_t reg_addr);
    int8_t _pos,_oldpos;
    uint16_t _touchreg;
    unsigned long starttime;
};