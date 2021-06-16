#ifndef __qsa72_aom_a_48p_sfp_plus_cpld_sysfs_H__
#define __qsa72_aom_a_48p_sfp_plus_cpld_sysfs_H__

/* Generic CPLD sysfs file definition macros */
#define SYSFS_RAW_RO_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, system_cpld_##field##_raw_read, NULL);

#define SYSFS_RAW_RW_ATTR_DEF(field)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUSR, system_cpld_##field##_raw_read, system_cpld_##field##_raw_write);

#define SYSFS_MISC_RO_ATTR_DEF(field, _read)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO, _read, NULL);

#define SYSFS_MISC_RW_ATTR_DEF(field, _read, _write)  \
struct device_attribute field  \
    = __ATTR(field, S_IRUGO | S_IWUSR, _read, _write);

#define SYSFS_ATTR_PTR(field) &field.attr

SYSFS_RAW_RW_ATTR_DEF(in_1                  )
SYSFS_RAW_RW_ATTR_DEF(in_2                  )
SYSFS_RAW_RW_ATTR_DEF(out_1                 )
SYSFS_RAW_RW_ATTR_DEF(out_2                 )
SYSFS_RAW_RW_ATTR_DEF(inv_1                 )
SYSFS_RAW_RW_ATTR_DEF(inv_2                 )
SYSFS_RAW_RW_ATTR_DEF(dir_1                 )
SYSFS_RAW_RW_ATTR_DEF(dir_2                 )
//SYSFS_RAW_RW_ATTR_DEF(debug                 )
SYSFS_RAW_RO_ATTR_DEF(p49_mod_abs                )
SYSFS_RAW_RO_ATTR_DEF(p50_mod_abs                )
SYSFS_RAW_RO_ATTR_DEF(p51_mod_abs                )
SYSFS_RAW_RO_ATTR_DEF(p52_mod_abs                )
SYSFS_RAW_RO_ATTR_DEF(p49_rx_los                 )
SYSFS_RAW_RO_ATTR_DEF(p50_rx_los                 )
SYSFS_RAW_RO_ATTR_DEF(p51_rx_los                 )
SYSFS_RAW_RO_ATTR_DEF(p52_rx_los                 )
//SYSFS_RAW_RO_ATTR_DEF(p49_tx_fault               )
//SYSFS_RAW_RO_ATTR_DEF(p50_tx_fault               )
//SYSFS_RAW_RO_ATTR_DEF(p51_tx_fault               )
//SYSFS_RAW_RO_ATTR_DEF(p52_tx_fault               )
SYSFS_RAW_RW_ATTR_DEF(p49_tx_disable             )
SYSFS_RAW_RW_ATTR_DEF(p50_tx_disable             )
SYSFS_RAW_RW_ATTR_DEF(p51_tx_disable             )
SYSFS_RAW_RW_ATTR_DEF(p52_tx_disable             )


#endif /*__qsa72_aom_a_48p_sfp_plus_cpld_sysfs_H__*/

