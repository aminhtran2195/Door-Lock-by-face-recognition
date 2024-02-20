#include "MyMPR121.h"
#include <Arduino.h>

uint16_t MPR121::ReadReg(uint8_t reg_addr){
    Wire.beginTransmission(MPR121_ADDR);
    Wire.write(reg_addr);
    Wire.endTransmission(false);
    Wire.requestFrom(MPR121_ADDR,1);    
    return Wire.read();
}

void MPR121::WriteReg(uint8_t reg_addr, uint8_t val){
    Wire.beginTransmission(MPR121_ADDR);
    Wire.write(reg_addr);
    Wire.write(val);
    Wire.endTransmission();
}

void MPR121::InitConfig(){
    _touchreg = 0;
    this->UpdateState(state::idle); // cần sửa lại 
    this->WriteReg(MHD_R,0x01);
    this->WriteReg(NHD_R, 0x01);
    this->WriteReg(NCL_R, 0x00);
    this->WriteReg(FDL_R, 0x00);

  // Section B
  // This group controls filtering when data is < baseline.
    this->WriteReg(MHD_F, 0x01);
    this->WriteReg(NHD_F, 0x01);
    this->WriteReg(NCL_F, 0xFF);
    this->WriteReg(FDL_F, 0x02);
  
  // Section C
  // This group sets touch and release thresholds for each electrode
    this->WriteReg(ELE0_T, TOU_THRESH);
    this->WriteReg(ELE0_R, REL_THRESH);
    this->WriteReg(ELE1_T, TOU_THRESH);
    this->WriteReg(ELE1_R, REL_THRESH);
    this->WriteReg(ELE2_T, TOU_THRESH);
    this->WriteReg(ELE2_R, REL_THRESH);
    this->WriteReg(ELE3_T, TOU_THRESH);
    this->WriteReg(ELE3_R, REL_THRESH);
    this->WriteReg(ELE4_T, TOU_THRESH);
    this->WriteReg(ELE4_R, REL_THRESH);
    this->WriteReg(ELE5_T, TOU_THRESH);
    this->WriteReg(ELE5_R, REL_THRESH);
    this->WriteReg(ELE6_T, TOU_THRESH);
    this->WriteReg(ELE6_R, REL_THRESH);
    this->WriteReg(ELE7_T, TOU_THRESH);
    this->WriteReg(ELE7_R, REL_THRESH);
    this->WriteReg(ELE8_T, TOU_THRESH);
    this->WriteReg(ELE8_R, REL_THRESH);
    this->WriteReg(ELE9_T, TOU_THRESH);
    this->WriteReg(ELE9_R, REL_THRESH);
    this->WriteReg(ELE10_T, TOU_THRESH);
    this->WriteReg(ELE10_R, REL_THRESH);
    this->WriteReg(ELE11_T, TOU_THRESH);
    this->WriteReg(ELE11_R, REL_THRESH);
  
  // Section D
  // Set the Filter Configuration
  // Set ESI2
    this->WriteReg(FIL_CFG, 0x04);
  
  // Section E
  // Electrode Configuration
    this->WriteReg(ELE_CFG, 0x0C);	// Enables all 12 Electrodes
}

void MPR121::UpdateTouchreg(){
    _touchreg = 0;
    _touchreg |= ReadReg(0x01) << 8;
    _touchreg |= ReadReg(0x00);
}

MPR121::MPR121(){
    _touchreg = 0;
    _pos = -1;
    this->UpdateState(state::idle);
}

char MPR121::getKey(){
  UpdateTouchreg();
    //Serial.println(touchstatus,BIN);
  for(int8_t i = 11; i >= 0;i--){
    if(_touchreg & (1<<i)) {
      _pos = i;
      //Serial.print("pos = ");
      //Serial.println(_pos);
      break;
    }
  }
  if (_touchreg != 0 && _pos != -1){
    return _Keych[_pos];  
  }
  return '\0';
}

void MPR121::UpdateState(state newstt){
  this->stt = newstt;         // cập nhật trạng thái mới
  this->starttime = millis(); // cập nhật lại thời gian chuyển trang thái
  if(newstt == state::idle) {
    _pos = -1;
    _oldpos = -1;
    cnt = 0;
  }
}

uint8_t MPR121::GetState(uint8_t mode = MULTICLK_OFF){
  UpdateTouchreg();
  switch (stt){
  case idle:
     if(_touchreg != 0) {
      Serial.println("Update press from idle");
      cnt++;
      this -> UpdateState(press);
      if(mode == MULTICLK_ON) return PRESSED;
     }
     break;
  case press:
    if(_touchreg == 0) {
      if(mode == MULTICLK_OFF){
        Serial.println("Update idle from press");
        this -> UpdateState(idle);
        return PRESSED;
      }
      else {
        Serial.println("Update released from press");
        this->UpdateState(released);
        _oldpos = _pos;
      }
    }
    else if((millis() - starttime) > 600 && mode == MULTICLK_OFF) {
      Serial.println("Update hold from press");
      this -> UpdateState(holding);
      return HOLD;
    }
    break;
  case holding:
    if(_touchreg == 0) {
      Serial.println("Update idle from holding");
      this -> UpdateState(idle);
    }
    break;
  case released:
    if((millis() - starttime) > 400 ) {
      Serial.println("Update idle from released");
      this->UpdateState(idle);
      return IDLE;
    } 
    else if(_touchreg != 0){
      if(_pos == _oldpos){
        Serial.println("Update multiclick from released");
        this -> UpdateState(press);
        cnt++;
        return PRESSED; 
      }
      else {
        Serial.println("Update idle from released, cuz pressed another key");
        this -> UpdateState(idle);
      }
    }  
  default: 
    break;
  }
  return NONE;
}

