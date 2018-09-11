//
//  macho_dlfcn.cpp
//
//  Created by zuopengliu on 5/9/2018.
//

#include "macho_dlfcn.h"
#include "MachOParser.h"
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <malloc/malloc.h>



#ifdef __LP64__

typedef struct mach_header_64       mach_header_t;
typedef struct segment_command_64   segment_command_t;
typedef struct section_64           section_t;
typedef struct nlist_64             nlist_t;

#define LC_SEGMENT_CMD              LC_SEGMENT_64

#else

typedef struct mach_header          mach_header_t;
typedef struct segment_command      segment_command_t;
typedef struct section              section_t;
typedef struct nlist                nlist_t;

#define LC_SEGMENT_CMD              LC_SEGMENT

#endif


using namespace NSMachO;


vm_address_t hmd_image_find_sym(const mach_header_t *image, vm_address_t vmaddr_slide, const char *symbol_name, bool *is_weak)
{
    if (!image) return 0;
    
    const struct symtab_command *symb_command = 0;
    const segment_command_t *linkedit_command = 0;
    
    vm_address_t cmd_ptr = (vm_address_t)(image + 1);
    for (uint32_t i_cmd = 0; i_cmd < image->ncmds; i_cmd++) {
        const struct load_command *command = (struct load_command *)cmd_ptr;
        if (command->cmd == LC_SYMTAB) {
            symb_command = (struct symtab_command *)command;
        } else if (command->cmd == LC_SEGMENT_CMD) {
            const segment_command_t *seg = (segment_command_t *)command;
            if (0 == strcmp(SEG_LINKEDIT, seg->segname)) {
                linkedit_command = seg;
            }
        }
        
        if (symb_command != 0 && linkedit_command != 0) {
            break;
        }
        cmd_ptr += command->cmdsize;
    }
    
    if (!symb_command || !linkedit_command) {
        return 0;
    }
    
    vm_address_t segment_base = linkedit_command->vmaddr - linkedit_command->fileoff + vmaddr_slide;
    
    const nlist_t *symbol_table = (nlist_t*)(segment_base + symb_command->symoff);
    const uintptr_t string_table = segment_base + symb_command->stroff;
    
    for (uint32_t i_sym = 0; i_sym < symb_command->nsyms; i_sym++)
    {
        nlist_t sym = symbol_table[i_sym];
        if (sym.n_value != 0)
        {
            const char *symname_addr = (char *)(string_table + sym.n_un.n_strx);
            if (strcmp(symbol_name, symname_addr) == 0) {
                if (is_weak) *is_weak = sym.n_desc & N_WEAK_DEF;
                return sym.n_value + vmaddr_slide;
            }
        }
    }
    
    return 0;
}

static const char *DYLD_SIM_INAME = "dyld_sim";

vm_address_t NSMachO::macho_lookup_sym(void *handler, const char *symbolName)
{
    if (!symbolName) return 0;
    vm_address_t addr = macho_dlsym(handler, symbolName);
    if (addr != 0) return addr;
    
    char underscoredName[strlen(symbolName)+2];
    underscoredName[0] = '_';
    strcpy(&underscoredName[1], symbolName);
    
    // 有BUG的方式查找符号
    const uint32_t image_count = _dyld_image_count();
    const mach_header_t *firstWeakImage = NULL;
    vm_address_t firstWeakSym = 0;
    
    const char *wname = NULL;
    for (uint32_t i = 0; i < image_count; i++) {
        bool isWeak = false;
        const char *iname = _dyld_get_image_name(i);
        const mach_header_t *header = (mach_header_t *)_dyld_get_image_header(i);
        vm_address_t address = hmd_image_find_sym(header, _dyld_get_image_vmaddr_slide(i), underscoredName, &isWeak);
        
        if (header->filetype == MH_DYLINKER &&
            iname && (strstr(iname, DYLD_SIM_INAME) != NULL)) {
            continue;
        }
        
        if (!isWeak && address != 0) {
            printf("The symbol (%s) at image (%s)\n", symbolName, iname);
            return address;
        }
        
        if (!firstWeakImage) {
            wname = (const char *)iname;
            firstWeakImage = header;
            firstWeakSym = address;
        }
    }
    
    if (firstWeakImage != NULL && firstWeakSym != 0 && wname)
        printf("The symbol (%s) at image (%s)\n", symbolName, wname);
    return ((firstWeakImage != NULL) ? firstWeakSym : 0);
}

#pragma mark -

vm_address_t NSMachO::macho_dlsym(void *handler, const char *symbolName)
{
    if (!symbolName) return 0;
    
    char underscoredName[strlen(symbolName)+2];
    underscoredName[0] = '_';
    strcpy(&underscoredName[1], symbolName);
    
    const uint32_t numOfImages = _dyld_image_count();
    // magic "search all in load order" handle
    for (uint32_t i = 0; i < numOfImages; ++i) {
        const mach_header *header = (mach_header *)_dyld_get_image_header(i);
        NSMachO::MachOParser parser((const mach_header *)header);
        void *result = NULL;
        if ( parser.hasExportedSymbol(underscoredName, NULL, &result) ) {
            return (vm_address_t)result;
        }
    }
    
    return 0;
}
