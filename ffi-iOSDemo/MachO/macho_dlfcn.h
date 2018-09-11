//
//  macho_dlfcn.h
//
//  Created by zuopengliu on 5/9/2018.
//

#ifndef macho_dlfcn_h
#define macho_dlfcn_h

#include <stdio.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/vm_types.h>
#include <malloc/malloc.h>



namespace NSMachO {

vm_address_t macho_lookup_sym(void *handler, const char *symbolName);


/**
 目前handler不生效，全局搜索，未找到返回0

 @param handler     文件句柄
 @param symbolName  符号名
 @return 符号地址
 */
vm_address_t macho_dlsym(void *handler, const char *symbolName);

}

    
#endif /* macho_dlfcn_h */
