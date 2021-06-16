#ifndef __qsa72_aom_a_48p_sfp_plus_cpld_reg_H__
#define __qsa72_aom_a_48p_sfp_plus_cpld_reg_H__


static int system_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char *reg_name);
static int system_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char *reg_name);

/* Generic CPLD read function */
#define FLD_RAW_RD_FUNC(_reg, _fld, _wdh) static ssize_t \
system_cpld_##_fld##_raw_read(struct device *dev, struct device_attribute *attr, char *buf) { \
    return system_cpld_raw_read(dev, attr, buf, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* Generic CPLD write function */
#define FLD_RAW_WR_FUNC(_reg, _fld, _wdh) static ssize_t \
system_cpld_##_fld##_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { \
    return system_cpld_raw_write(dev, attr, buf, count, _reg##_offset, _reg##_width, _fld##_shift, _fld##_width, _fld##_mask, #_reg); \
}

/* CPLD register definition macros */
#define REG_DEF(_reg, _off, _wdh) \
static unsigned int _reg##_offset = (unsigned int)(_off); \
static unsigned int _reg##_width = (unsigned int)(_wdh);

/* CPLD register field definition macros, with generic read/write function */
#define FLD_RAW_RO_DEF(_reg, _fld, _sft, _wdh) \
static unsigned int _fld##_shift = (unsigned int)(_sft); \
static unsigned int _fld##_width = (unsigned int)(_wdh); \
static unsigned int _fld##_mask = ((((unsigned int)1) << (_wdh)) - 1); \
FLD_RAW_RD_FUNC(_reg, _fld, _wdh)

#define FLD_RAW_RW_DEF(_reg, _fld, _sft, _wdh) \
static unsigned int _fld##_shift = (unsigned int)(_sft); \
static unsigned int _fld##_width = (unsigned int)(_wdh); \
static unsigned int _fld##_mask = ((((unsigned int)1) << (_wdh)) - 1); \
FLD_RAW_RD_FUNC(_reg, _fld, _wdh) FLD_RAW_WR_FUNC(_reg, _fld, _wdh)


/* Declare system CPLD registers */
/*           register name                          offset  width */
/*           --------------------------------      -------  ----- */
REG_DEF(     in_1_reg   ,                             0x00,   8)
REG_DEF(     in_2_reg   ,                             0x01,   8)
REG_DEF(     out_1_reg  ,                             0x02,   8)
REG_DEF(     out_2_reg  ,                             0x03,   8)
REG_DEF(     inv_1_reg  ,                             0x04,   8)
REG_DEF(     inv_2_reg  ,                             0x05,   8)
REG_DEF(     dir_1_reg  ,                             0x06,   8)
REG_DEF(     dir_2_reg  ,                             0x07,   8)
//REG_DEF(     debug_reg  ,                             0xFF,   8)

/* Declare system CPLD register's fields */
/*                      register name               field name                  shift  width */
/*                      ----------------------      ----------------            ------ ----- */
FLD_RAW_RW_DEF(         in_1_reg,                   in_1,                       0,      8)
FLD_RAW_RW_DEF(         in_2_reg,                   in_2,                       0,      8)
FLD_RAW_RW_DEF(         out_1_reg,                  out_1,                      0,      8)
FLD_RAW_RW_DEF(         out_2_reg,                  out_2,                      0,      8)
FLD_RAW_RW_DEF(         inv_1_reg,                  inv_1,                      0,      8)
FLD_RAW_RW_DEF(         inv_2_reg,                  inv_2,                      0,      8)
FLD_RAW_RW_DEF(         dir_1_reg,                  dir_1,                      0,      8)
FLD_RAW_RW_DEF(         dir_2_reg,                  dir_2,                      0,      8)
//FLD_RAW_RW_DEF(         debug_reg,                  debug,                      0,      8)
FLD_RAW_RO_DEF(         in_1_reg,                   p49_mod_abs,                1,      1)
FLD_RAW_RO_DEF(         in_1_reg,                   p50_mod_abs,                4,      1)
FLD_RAW_RO_DEF(         in_1_reg,                   p51_mod_abs,                7,      1)
FLD_RAW_RO_DEF(         in_2_reg,                   p52_mod_abs,                2,      1)
FLD_RAW_RO_DEF(         in_1_reg,                   p49_rx_los,                 2,      1)
FLD_RAW_RO_DEF(         in_1_reg,                   p50_rx_los,                 5,      1)
FLD_RAW_RO_DEF(         in_2_reg,                   p51_rx_los,                 0,      1)
FLD_RAW_RO_DEF(         in_2_reg,                   p52_rx_los,                 3,      1)
FLD_RAW_RW_DEF(         out_1_reg,                  p49_tx_disable,             0,      1)
FLD_RAW_RW_DEF(         out_1_reg,                  p50_tx_disable,             3,      1)
FLD_RAW_RW_DEF(         out_1_reg,                  p51_tx_disable,             4,      1)
FLD_RAW_RW_DEF(         out_2_reg,                  p52_tx_disable,             1,      1)


#endif /* __qsa72_aom_a_48p_sfp_plus_cpld_reg_H__ */
