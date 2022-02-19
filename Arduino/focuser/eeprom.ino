void adjustEEPROMFocOffset() {
  long pos1 = readFocuserPos(0);
  long pos2 = readFocuserPos(1);
  int offset = ctx.focuserEEPROMoffset;
  offset++;
  if(offset > 19) offset = 0;
  ctx.focuserEEPROMoffset = offset; saveConfig();
  EepromUtil::eeprom_write_long(getFocuserEEPROMAaddress(0), pos1);
  EepromUtil::eeprom_write_long(getFocuserEEPROMAaddress(1), pos2);
}

void saveFocuserPos(long newPos, byte index) {
   EepromUtil::eeprom_write_long(getFocuserEEPROMAaddress(index), newPos);
}

long readFocuserPos(byte index) {
  long pos;
  EepromUtil::eeprom_read_long(getFocuserEEPROMAaddress(index), &pos);
  return pos;
}

int getFocuserEEPROMAaddress(byte index) {
  return motors[index].focuserPosStart + ctx.focuserEEPROMoffset * 4;
}

void printOwnerNick(char *answer) {
  char buf[16];
  EepromUtil::eeprom_read_string(NICK_START, buf, 15);
  strcat(answer, buf);
}

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default ctx.
  if (//EEPROM.read(CONFIG_START + sizeof(settings) - 1) == ctx.version_of_program[3] // this is '\0'
    EEPROM.read(CONFIG_START + sizeof(ctx) - 2) == ctx.version_of_program[2] &&
    EEPROM.read(CONFIG_START + sizeof(ctx) - 3) == ctx.version_of_program[1] &&
    EEPROM.read(CONFIG_START + sizeof(ctx) - 4) == ctx.version_of_program[0])
  { // reads settings from EEPROM
    for (unsigned int t = 0; t < sizeof(ctx); t++)
      *((char*)&ctx + t) = EEPROM.read(CONFIG_START + t);
  } else {
    // settings aren't valid! will overwrite with default settings
    saveConfig();
  }
}

void saveConfig() {
  for (unsigned int t = 0; t < sizeof(ctx); t++)
  { // writes to EEPROM if value is different
    if (EEPROM.read(CONFIG_START + t) != *((char*)&ctx + t))
      EEPROM.write(CONFIG_START + t, *((char*)&ctx + t));
  }
}

