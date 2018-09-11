//
//  MachOParser.h
//
//  Created by zuopengliu on 4/9/2018.
//  Copyright © 2018 zuopengl. All rights reserved.
//

#ifndef MachOParser_h
#define MachOParser_h

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <set>
#include <vector>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach/mach.h>
#include <malloc/malloc.h>
#include <mach/vm_types.h>


#define DYLD_IN_PROCESS                 1

#pragma mark -

namespace NSMachO { // BEGIN namespace NSMachO


enum class Platform {
    unknown     = 0,
    macOS       = 1,
    iOS         = 2,
    tvOS        = 3,
    watchOS     = 4,
    bridgeOS    = 5
};

class Diagnostics
{
public:
    Diagnostics(const std::string& prefix, bool verbose=true);
    Diagnostics(bool verbose=false);
    ~Diagnostics();
    
    void            error(const char* format, ...)  __attribute__((format(printf, 2, 3)));
    
    void            assertNoError() const;
    bool            hasError() const;
    bool            noError() const;
    void            clearError();
    
    const std::string               prefix() const;
    std::string                     errorMessage() const;
    
private:
    void*                    _buffer = nullptr;
    std::string              _prefix;
    std::set<std::string>    _errors;
    bool                     _verbose = false;
};


#pragma mark -

/**
 @note:
    MachOParser 仅仅适用与加载到内存中的Image
 */
class MachOParser
{
public:
    // 构造函数
    MachOParser(const mach_header* mh, bool dyldCacheIsRaw=false);
    
public:
    static bool         isMachO(Diagnostics& diag, const void* fileContent, size_t fileLength);
    static bool         wellFormedMachHeaderAndLoadCommands(const mach_header* mh);
   
public:
    const mach_header*  header() const;
    uint32_t            fileType() const;
    std::string         archName() const;
    bool                is64() const;
    bool                inDyldCache() const;
    bool                hasThreadLocalVariables() const;
    Platform            platform() const;
    uint64_t            preferredLoadAddress() const;
    bool                getUuid(uuid_t uuid) const;
    bool                getPlatformAndVersion(Platform* platform, uint32_t* minOS, uint32_t* sdk) const;
    bool                isSimulatorBinary() const;
    bool                getDylibInstallName(const char** installName, uint32_t* compatVersion, uint32_t* currentVersion) const;
    const char*         installName() const;
    uint32_t            dependentDylibCount() const;
    const char*         dependentDylibLoadPath(uint32_t depIndex) const;
    void                forEachDependentDylib(void (^callback)(const char* loadPath, bool isWeak, bool isReExport, bool isUpward, uint32_t compatVersion, uint32_t curVersion, bool& stop)) const;
    void                forEachSection(void (^callback)(const char* segName, const char* sectionName, uint32_t flags, const void* content, size_t size, bool illegalSectionSize, bool& stop)) const;
    void                forEachSegment(void (^callback)(const char* segName, uint32_t fileOffset, uint32_t fileSize, uint64_t vmAddr, uint64_t vmSize, uint8_t protections, bool& stop)) const;
    void                forEachGlobalSymbol(Diagnostics& diag, void (^callback)(const char* symbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop)) const;
    void                forEachLocalSymbol(Diagnostics& diag, void (^callback)(const char* symbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop)) const;
    void                forEachRPath(void (^callback)(const char* rPath, bool& stop)) const;
    void                forEachSection(void (^callback)(const char* segName, const char* sectionName, uint32_t flags, uint64_t addr, const void* content,
                                                        uint64_t size, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2, bool illegalSectionSize, bool& stop)) const;
    
public:
    struct FoundSymbol {
        enum class Kind { headerOffset, absolute, resolverOffset };
        Kind                kind;
        bool                isThreadLocal;
        const mach_header*  foundInDylib;
        void*               foundExtra;
        uint64_t            value;
        uint32_t            resolverFuncOffset;
        const char*         foundSymbolName;
    };
    
    typedef bool (^DependentFinder)(uint32_t depIndex, const char* depLoadPath, void* extra, const mach_header** foundMH, void** foundExtra);
    
    bool                findExportedSymbol(Diagnostics& diag, const char* symbolName, void* extra, FoundSymbol& foundInfo, DependentFinder finder) const;
    bool                findClosestSymbol(uint64_t unSlidAddr, const char** symbolName, uint64_t* symbolUnslidAddr) const;
    bool                isFairPlayEncrypted(uint32_t& textOffset, uint32_t& size);
    
    intptr_t            getSlide() const;
    bool                hasExportedSymbol(const char* symbolName, DependentFinder finder, void** result) const;
    bool                findClosestSymbol(const void* addr, const char** symbolName, const void** symbolAddress) const;
    const char*         segmentName(uint32_t segIndex) const;
    
public:
    static const uint8_t*   trieWalk(Diagnostics& diag, const uint8_t* start, const uint8_t* end, const char* symbol);
    static uint64_t         read_uleb128(Diagnostics& diag, const uint8_t*& p, const uint8_t* end);
    static int64_t          read_sleb128(Diagnostics& diag, const uint8_t*& p, const uint8_t* end);
    
private:
    struct LayoutInfo {
#if DYLD_IN_PROCESS
        uintptr_t    slide;
        uintptr_t    textUnslidVMAddr;
        uintptr_t    linkeditUnslidVMAddr;
        uint32_t     linkeditFileOffset;
#else
        uint32_t     segmentCount;
        uint32_t     linkeditSegIndex;
        struct {
            uint64_t    mappingOffset;
            uint64_t    fileOffset;
            uint64_t    fileSize;
            uint64_t    segUnslidAddress;
            uint64_t    writable          :  1,
            executable        :  1,
            textRelocsAllowed :  1,  // segment supports text relocs (i386 only)
            segSize           : 61;
        }            segments[128];
#endif
    };
    
    struct LinkEditInfo {
        const dyld_info_command*     dyldInfo;
        const symtab_command*        symTab;
        const dysymtab_command*      dynSymTab;
        const segment_command*       linkeditCmd;
        const linkedit_data_command* splitSegInfo;
        const linkedit_data_command* functionStarts;
        const linkedit_data_command* dataInCode;
        const linkedit_data_command* codeSig;
        LayoutInfo                   layout;
    };
    
    void                    getLinkEditPointers(Diagnostics& diag, LinkEditInfo&) const;
    void                    getLinkEditLoadCommands(Diagnostics& diag, LinkEditInfo& result) const;
    void                    getLayoutInfo(LayoutInfo&) const;
    const uint8_t*          getLinkEditContent(const LayoutInfo& info, uint32_t fileOffset) const;
    
    
private:
    void                    forEachSection(void (^callback)(const char* segName, uint32_t segIndex, uint64_t segVMAddr, const char* sectionName, uint32_t sectFlags,
                                                            uint64_t sectAddr, uint64_t size, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2, bool illegalSectionSize, bool& stop)) const;
    
    void                    forEachLoadCommand(Diagnostics& diag, void (^callback)(const load_command* cmd, bool& stop)) const;
    bool                    isRaw() const;
    bool                    inRawCache() const;
    
private:
    long                _data; // if low bit true, then this is raw file (not loaded image)
};

    
} // END namespace NSMachO

#endif /* MachOParser_h */
