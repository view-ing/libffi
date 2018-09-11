//
//  MachOParser.cpp
//
//  Created by zuopengliu on 4/9/2018.
//  Copyright © 2018 zuopengl. All rights reserved.
//

#include "MachOParser.h"
#include <assert.h>


namespace NSMachO { // BEGIN namespace NSMachO

    
#define BD_BIND_TYPE_IMPORT_JMP_REL32   4

/// Returns true if (addLHS + addRHS) > b, or if the add overflowed
template<typename T>
static bool greaterThanAddOrOverflow(uint64_t addLHS, uint64_t addRHS, T b) {
    return (addLHS > b) || (addRHS > (b-addLHS));
}

inline bool endsWith(const std::string& str, const std::string& suffix)
{
    std::size_t index = str.find(suffix, str.size() - suffix.size());
    return (index != std::string::npos);
}

#pragma mark -

Diagnostics::Diagnostics(bool verbose)
: _verbose(verbose)
, _prefix("")
{
}

Diagnostics::Diagnostics(const std::string& prefix, bool verbose)
: _verbose(verbose)
, _prefix(prefix)
{
}

Diagnostics::~Diagnostics()
{
    clearError();
}

void Diagnostics::error(const char* format, ...)
{
    char *tmperr = NULL;
    va_list    list;
    va_start(list, format);
    vasprintf(&tmperr, format, list);
    va_end(list);
    if (!tmperr) return;
    _errors.insert(std::string(tmperr));
    
    if ( !_verbose ) {
        if (tmperr) free(tmperr);
        return;
    }
    
    if (_prefix.empty()) {
        fprintf(stderr, "%s", tmperr);
    } else {
        fprintf(stderr, "[%s] %s", _prefix.c_str(), tmperr);
    }
    
    if (tmperr) free(tmperr);
}

void Diagnostics::assertNoError() const
{
#if DEBUG
    assert(noError() && "appear error");
#endif
}
bool Diagnostics::hasError() const
{
    return _buffer != nullptr || !_errors.empty();
}

bool Diagnostics::noError() const
{
    return !hasError();
}

void Diagnostics::clearError()
{
    if ( _buffer ) free(_buffer);
    _buffer = nullptr;
}

const std::string Diagnostics::prefix() const
{
    return _prefix;
}

std::string Diagnostics::errorMessage() const
{
    std::string str;
    if (_buffer != nullptr) {
        str.append((char *)_buffer);
        str.push_back('\n');
    }

    for (std::set<std::string>::iterator itr = _errors.begin(); itr != _errors.end(); itr++) {
        str.append(*itr);
        str.push_back('\n');
    }
    return str;
}



#pragma mark - MachOParser

MachOParser::MachOParser(const mach_header* mh, bool dyldCacheIsRaw)
{
#if DYLD_IN_PROCESS
    // assume all in-process mach_headers are real loaded images
    _data = (long)mh;
#else
    if (mh == nullptr)
        return;
    _data = (long)mh;
    if ( (mh->flags & 0x80000000) == 0 ) {
        // asssume out-of-process mach_header not in a dyld cache are raw mapped files
        _data |= 1;
    }
    else {
        // out-of-process mach_header in a dyld cache are not raw, but cache may be raw
        if ( dyldCacheIsRaw )
            _data |= 2;
    }
#endif
}

const mach_header* MachOParser::header() const
{
    return (mach_header*)(_data & -4);
}

// "raw" means the whole mach-o file was mapped as one contiguous region
// not-raw means the the mach-o file was mapped like dyld does - with zero fill expansion
bool MachOParser::isRaw() const
{
    return (_data & 1);
}

// A raw dyld cache is when the whole dyld cache file is mapped in one contiguous region
// not-raw manes the dyld cache was mapped as it is at runtime with padding between regions
bool MachOParser::inRawCache() const
{
    return (_data & 2);
}

uint32_t MachOParser::fileType() const
{
    return header()->filetype;
}

bool MachOParser::inDyldCache() const
{
    return (header()->flags & 0x80000000);
}

bool MachOParser::hasThreadLocalVariables() const
{
    return (header()->flags & MH_HAS_TLV_DESCRIPTORS);
}

Platform MachOParser::platform() const
{
    Platform platform;
    uint32_t minOS;
    uint32_t sdk;
    if ( getPlatformAndVersion(&platform, &minOS, &sdk) )
        return platform;
    
    // old binary with no explict load command to mark platform, look at arch
    switch ( header()->cputype ) {
        case CPU_TYPE_X86_64:
        case CPU_TYPE_I386:
            return Platform::macOS;
        case CPU_TYPE_ARM64:
        case CPU_TYPE_ARM:
            return Platform::iOS;
    }
    return Platform::macOS;
}


#if !DYLD_IN_PROCESS

const MachOParser::ArchInfo MachOParser::_s_archInfos[] = {
    { "x86_64",     CPU_TYPE_X86_64,    CPU_SUBTYPE_X86_64_ALL },
    { "x86_64h",    CPU_TYPE_X86_64,    CPU_SUBTYPE_X86_64_H },
    { "i386",       CPU_TYPE_I386,      CPU_SUBTYPE_I386_ALL },
    { "arm64",      CPU_TYPE_ARM64,     CPU_SUBTYPE_ARM64_ALL },
    { "arm64_v8",   CPU_TYPE_ARM64,     CPU_SUBTYPE_ARM64_V8 },
    
    { "armv4t",     CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V4T },
    { "armv6",      CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V6 },
    { "armv5tej",   CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V5TEJ },
    { "armxscale",  CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_XSCALE },
    { "armv7f",     CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V7F },
    { "armv7s",     CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V7S },
    { "armv7k",     CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V7K },
    { "armv7",      CPU_TYPE_ARM,       CPU_SUBTYPE_ARM_V7 },
};

#else

bool MachOParser::isMachO(Diagnostics& diag, const void* fileContent, size_t mappedLength)
{
    // sanity check length
    if ( mappedLength < 4096 ) {
        diag.error("file too short");
        return false;
    }
    
    // must start with mach-o magic value
    const mach_header* mh = (const mach_header*)fileContent;
#if __LP64__
    const uint32_t requiredMagic = MH_MAGIC_64;
#else
    const uint32_t requiredMagic = MH_MAGIC;
#endif
    if ( mh->magic != requiredMagic ) {
        diag.error("not a mach-o file");
        return false;
    }
    
#if __x86_64__
    const uint32_t requiredCPU = CPU_TYPE_X86_64;
#elif __i386__
    const uint32_t requiredCPU = CPU_TYPE_I386;
#elif __arm__
    const uint32_t requiredCPU = CPU_TYPE_ARM;
#elif __arm64__
    const uint32_t requiredCPU = CPU_TYPE_ARM64;
#else
#error unsupported architecture
#endif
    if ( mh->cputype != requiredCPU ) {
        diag.error("wrong cpu type");
        return false;
    }
    
    return true;
}

bool MachOParser::wellFormedMachHeaderAndLoadCommands(const mach_header* mh)
{
    const load_command* startCmds = nullptr;
    if ( mh->magic == MH_MAGIC_64 )
        startCmds = (load_command*)((char *)mh + sizeof(mach_header_64));
    else if ( mh->magic == MH_MAGIC )
        startCmds = (load_command*)((char *)mh + sizeof(mach_header));
    else
        return false;  // not a mach-o file, or wrong endianness
    
    const load_command* const cmdsEnd = (load_command*)((char*)startCmds + mh->sizeofcmds);
    const load_command* cmd = startCmds;
    for(uint32_t i = 0; i < mh->ncmds; ++i) {
        const load_command* nextCmd = (load_command*)((char *)cmd + cmd->cmdsize);
        if ( (cmd->cmdsize < 8) || (nextCmd > cmdsEnd) || (nextCmd < startCmds)) {
            return false;
        }
        cmd = nextCmd;
    }
    return true;
}

#endif

//Platform MachOParser::currentPlatform()
//{
//#if TARGET_OS_BRIDGE
//    return Platform::bridgeOS;
//#elif TARGET_OS_WATCH
//    return Platform::watchOS;
//#elif TARGET_OS_TV
//    return Platform::tvOS;
//#elif TARGET_OS_IOS
//    return Platform::iOS;
//#elif TARGET_OS_MAC
//    return Platform::macOS;
//#else
//#error unknown platform
//#endif
//}


void MachOParser::forEachLoadCommand(Diagnostics& diag, void (^callback)(const load_command* cmd, bool& stop)) const
{
    bool stop = false;
    const load_command* startCmds = nullptr;
    if ( header()->magic == MH_MAGIC_64 )
        startCmds = (load_command*)((char *)header() + sizeof(mach_header_64));
    else if ( header()->magic == MH_MAGIC )
        startCmds = (load_command*)((char *)header() + sizeof(mach_header));
    else {
        diag.error("file does not start with MH_MAGIC[_64]");
        return;  // not a mach-o file, or wrong endianness
    }
    const load_command* const cmdsEnd = (load_command*)((char*)startCmds + header()->sizeofcmds);
    const load_command* cmd = startCmds;
    for(uint32_t i = 0; i < header()->ncmds; ++i) {
        const load_command* nextCmd = (load_command*)((char *)cmd + cmd->cmdsize);
        if ( cmd->cmdsize < 8 ) {
            diag.error("malformed load command #%d, size too small %d", i, cmd->cmdsize);
            return;
        }
        if ( (nextCmd > cmdsEnd) || (nextCmd < startCmds) ) {
            diag.error("malformed load command #%d, size too large 0x%X", i, cmd->cmdsize);
            return;
        }
        callback(cmd, stop);
        if ( stop )
            return;
        cmd = nextCmd;
    }
}

uint64_t MachOParser::preferredLoadAddress() const
{
    __block uint64_t result = 0;
    forEachSegment(^(const char* segName, uint32_t fileOffset, uint32_t fileSize, uint64_t vmAddr, uint64_t vmSize, uint8_t protections, bool& stop) {
        if ( strcmp(segName, "__TEXT") == 0 ) {
            result = vmAddr;
            stop = true;
        }
    });
    return result;
}

bool MachOParser::getPlatformAndVersion(Platform* platform, uint32_t* minOS, uint32_t* sdk) const
{
    Diagnostics diag;
    __block bool found = false;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        const version_min_command* versCmd;
        switch ( cmd->cmd ) {
            case LC_VERSION_MIN_IPHONEOS:
                versCmd       = (version_min_command*)cmd;
                *platform     = Platform::iOS;
                *minOS        = versCmd->version;
                *sdk          = versCmd->sdk;
                found = true;
                stop = true;
                break;
            case LC_VERSION_MIN_MACOSX:
                versCmd       = (version_min_command*)cmd;
                *platform     = Platform::macOS;
                *minOS        = versCmd->version;
                *sdk          = versCmd->sdk;
                found = true;
                stop = true;
                break;
            case LC_VERSION_MIN_TVOS:
                versCmd       = (version_min_command*)cmd;
                *platform     = Platform::tvOS;
                *minOS        = versCmd->version;
                *sdk          = versCmd->sdk;
                found = true;
                stop = true;
                break;
            case LC_VERSION_MIN_WATCHOS:
                versCmd       = (version_min_command*)cmd;
                *platform     = Platform::watchOS;
                *minOS        = versCmd->version;
                *sdk          = versCmd->sdk;
                found = true;
                stop = true;
                break;
            case LC_BUILD_VERSION: {
                const build_version_command* buildCmd = (build_version_command *)cmd;
                *minOS        = buildCmd->minos;
                *sdk          = buildCmd->sdk;
                
                switch(buildCmd->platform) {
                        /* Known values for the platform field above. */
                    case PLATFORM_MACOS:
                        *platform = Platform::macOS;
                        break;
                    case PLATFORM_IOS:
                        *platform = Platform::iOS;
                        break;
                    case PLATFORM_TVOS:
                        *platform = Platform::tvOS;
                        break;
                    case PLATFORM_WATCHOS:
                        *platform = Platform::watchOS;
                        break;
                    case PLATFORM_BRIDGEOS:
                        *platform = Platform::bridgeOS;
                        break;
                }
                found = true;
                stop = true;
            } break;
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
    return found;
}


bool MachOParser::isSimulatorBinary() const
{
    Platform platform;
    uint32_t minOS;
    uint32_t sdk;
    switch ( header()->cputype ) {
        case CPU_TYPE_I386:
        case CPU_TYPE_X86_64:
            if ( getPlatformAndVersion(&platform, &minOS, &sdk) ) {
                return (platform != Platform::macOS);
            }
            break;
    }
    return false;
}


bool MachOParser::getDylibInstallName(const char** installName, uint32_t* compatVersion, uint32_t* currentVersion) const
{
    Diagnostics diag;
    __block bool found = false;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        if ( cmd->cmd == LC_ID_DYLIB ) {
            const dylib_command*  dylibCmd = (dylib_command*)cmd;
            *compatVersion  = dylibCmd->dylib.compatibility_version;
            *currentVersion = dylibCmd->dylib.current_version;
            *installName    = (char*)dylibCmd + dylibCmd->dylib.name.offset;
            found = true;
            stop = true;
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
    return found;
}

const char* MachOParser::installName() const
{
    assert(header()->filetype == MH_DYLIB);
    const char* result;
    uint32_t    ignoreVersion;
    assert(getDylibInstallName(&result, &ignoreVersion, &ignoreVersion));
    return result;
}


uint32_t MachOParser::dependentDylibCount() const
{
    __block uint32_t count = 0;
    forEachDependentDylib(^(const char* loadPath, bool isWeak, bool isReExport, bool isUpward, uint32_t compatVersion, uint32_t curVersion, bool& stop) {
        ++count;
    });
    return count;
}

const char* MachOParser::dependentDylibLoadPath(uint32_t depIndex) const
{
    __block const char* foundLoadPath = nullptr;
    __block uint32_t curDepIndex = 0;
    forEachDependentDylib(^(const char* loadPath, bool isWeak, bool isReExport, bool isUpward, uint32_t compatVersion, uint32_t curVersion, bool& stop) {
        if ( curDepIndex == depIndex ) {
            foundLoadPath = loadPath;
            stop = true;
        }
        ++curDepIndex;
    });
    return foundLoadPath;
}


void MachOParser::forEachDependentDylib(void (^callback)(const char* loadPath, bool isWeak, bool isReExport, bool isUpward, uint32_t compatVersion, uint32_t curVersion, bool& stop)) const
{
    Diagnostics diag;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        switch ( cmd->cmd ) {
            case LC_LOAD_DYLIB:
            case LC_LOAD_WEAK_DYLIB:
            case LC_REEXPORT_DYLIB:
            case LC_LOAD_UPWARD_DYLIB: {
                const dylib_command* dylibCmd = (dylib_command*)cmd;
                assert(dylibCmd->dylib.name.offset < cmd->cmdsize);
                const char* loadPath = (char*)dylibCmd + dylibCmd->dylib.name.offset;
                callback(loadPath, (cmd->cmd == LC_LOAD_WEAK_DYLIB), (cmd->cmd == LC_REEXPORT_DYLIB), (cmd->cmd == LC_LOAD_UPWARD_DYLIB),
                         dylibCmd->dylib.compatibility_version, dylibCmd->dylib.current_version, stop);
            }
                break;
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
}

void MachOParser::forEachRPath(void (^callback)(const char* rPath, bool& stop)) const
{
    Diagnostics diag;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        if ( cmd->cmd == LC_RPATH ) {
            const char* rpath = (char*)cmd + ((struct rpath_command*)cmd)->path.offset;
            callback(rpath, stop);
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
}

/*
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
 uint64_t    segUnslidAddress;
 uint64_t    segSize;
 }            segments[16];
 #endif
 };
 */

const uint8_t* MachOParser::getLinkEditContent(const LayoutInfo& info, uint32_t fileOffset) const
{
#if DYLD_IN_PROCESS
    uint32_t offsetInLinkedit   = fileOffset - info.linkeditFileOffset;
    uintptr_t linkeditStartAddr = info.linkeditUnslidVMAddr + info.slide;
    return (uint8_t*)(linkeditStartAddr + offsetInLinkedit);
#else
    uint32_t offsetInLinkedit    = fileOffset - (uint32_t)(info.segments[info.linkeditSegIndex].fileOffset);
    const uint8_t* linkeditStart = (uint8_t*)header() + info.segments[info.linkeditSegIndex].mappingOffset;
    return linkeditStart + offsetInLinkedit;
#endif
}


void MachOParser::getLayoutInfo(LayoutInfo& result) const
{
#if DYLD_IN_PROCESS
    // image loaded by dyld, just record the addr and file offset of TEXT and LINKEDIT segments
    result.slide = getSlide();
    forEachSegment(^(const char* segName, uint32_t fileOffset, uint32_t fileSize, uint64_t vmAddr, uint64_t vmSize, uint8_t protections, bool& stop) {
        if ( strcmp(segName, "__TEXT") == 0 ) {
            result.textUnslidVMAddr = (uintptr_t)vmAddr;
        }
        else if ( strcmp(segName, "__LINKEDIT") == 0 ) {
            result.linkeditUnslidVMAddr = (uintptr_t)vmAddr;
            result.linkeditFileOffset   = fileOffset;
        }
    });
#else
    bool inCache = inDyldCache();
    bool intel32 = (header()->cputype == CPU_TYPE_I386);
    result.segmentCount = 0;
    result.linkeditSegIndex = 0xFFFFFFFF;
    __block uint64_t textSegAddr = 0;
    __block uint64_t textSegFileOffset = 0;
    forEachSegment(^(const char* segName, uint32_t fileOffset, uint32_t fileSize, uint64_t vmAddr, uint64_t vmSize, uint8_t protections, bool& stop) {
        auto& segInfo = result.segments[result.segmentCount];
        if ( strcmp(segName, "__TEXT") == 0 ) {
            textSegAddr       = vmAddr;
            textSegFileOffset = fileOffset;
        }
        __block bool textRelocsAllowed = false;
        if ( intel32 ) {
            forEachSection(^(const char* curSegName, uint32_t segIndex, uint64_t segVMAddr, const char* sectionName, uint32_t sectFlags,
                             uint64_t sectAddr, uint64_t size, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2, bool illegalSectionSize, bool& sectStop) {
                if ( strcmp(curSegName, segName) == 0 ) {
                    if ( sectFlags & (S_ATTR_EXT_RELOC|S_ATTR_LOC_RELOC) ) {
                        textRelocsAllowed = true;
                        sectStop = true;
                    }
                }
            });
        }
        if ( inCache ) {
            if ( inRawCache() ) {
                // whole cache file mapped somewhere (padding not expanded)
                // vmaddrs are useless. only file offset make sense
                segInfo.mappingOffset = fileOffset - textSegFileOffset;
            }
            else {
                // cache file was loaded by dyld into shared region
                // vmaddrs of segments are correct except for ASLR slide
                segInfo.mappingOffset = vmAddr - textSegAddr;
            }
        }
        else {
            // individual mach-o file mapped in one region, so mappingOffset == fileOffset
            segInfo.mappingOffset    = fileOffset;
        }
        segInfo.fileOffset        = fileOffset;
        segInfo.fileSize          = fileSize;
        segInfo.segUnslidAddress  = vmAddr;
        segInfo.segSize           = vmSize;
        segInfo.writable          = ((protections & VM_PROT_WRITE)   == VM_PROT_WRITE);
        segInfo.executable        = ((protections & VM_PROT_EXECUTE) == VM_PROT_EXECUTE);
        segInfo.textRelocsAllowed = textRelocsAllowed;
        if ( strcmp(segName, "__LINKEDIT") == 0 ) {
            result.linkeditSegIndex = result.segmentCount;
        }
        ++result.segmentCount;
        if ( result.segmentCount > 127 )
            stop = true;
    });
#endif
}


void MachOParser::forEachSection(void (^callback)(const char* segName, const char* sectionName, uint32_t flags,
                                                  const void* content, size_t size, bool illegalSectionSize, bool& stop)) const
{
    forEachSection(^(const char* segName, const char* sectionName, uint32_t flags, uint64_t addr,
                     const void* content, uint64_t size, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2, bool illegalSectionSize, bool& stop) {
        callback(segName, sectionName, flags, content, (size_t)size, illegalSectionSize, stop);
    });
}

void MachOParser::forEachSection(void (^callback)(const char* segName, const char* sectionName, uint32_t flags, uint64_t addr,
                                                  const void* content, uint64_t size, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2,
                                                  bool illegalSectionSize, bool& stop)) const
{
    Diagnostics diag;
    //fprintf(stderr, "forEachSection() mh=%p\n", header());
    LayoutInfo layout;
    getLayoutInfo(layout);
    forEachSection(^(const char* segName, uint32_t segIndex, uint64_t segVMAddr, const char* sectionName, uint32_t sectFlags,
                     uint64_t sectAddr, uint64_t sectSize, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2, bool illegalSectionSize, bool& stop) {
#if DYLD_IN_PROCESS
        const uint8_t* segContentStart = (uint8_t*)(segVMAddr + layout.slide);
#else
        const uint8_t* segContentStart = (uint8_t*)header() + layout.segments[segIndex].mappingOffset;
#endif
        const void* contentAddr = segContentStart + (sectAddr - segVMAddr);
        callback(segName, sectionName, sectFlags, sectAddr, contentAddr, sectSize, alignP2, reserved1, reserved2, illegalSectionSize, stop);
    });
    
}

// this iterator just walks the segment/section array.  It does interpret addresses
void MachOParser::forEachSection(void (^callback)(const char* segName, uint32_t segIndex, uint64_t segVMAddr, const char* sectionName, uint32_t sectFlags,
                                                  uint64_t sectAddr, uint64_t size, uint32_t alignP2, uint32_t reserved1, uint32_t reserved2, bool illegalSectionSize, bool& stop)) const
{
    Diagnostics diag;
    //fprintf(stderr, "forEachSection() mh=%p\n", header());
    __block uint32_t segIndex = 0;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        if ( cmd->cmd == LC_SEGMENT_64 ) {
            const segment_command_64* seg = (segment_command_64*)cmd;
            const section_64* const sectionsStart = (section_64*)((char*)seg + sizeof(struct segment_command_64));
            const section_64* const sectionsEnd   = &sectionsStart[seg->nsects];
            for (const section_64* sect=sectionsStart; !stop && (sect < sectionsEnd); ++sect) {
                const char* sectName = sect->sectname;
                char sectNameCopy[20];
                if ( sectName[15] != '\0' ) {
                    strlcpy(sectNameCopy, sectName, 17);
                    sectName = sectNameCopy;
                }
                bool illegalSectionSize = (sect->addr < seg->vmaddr) || greaterThanAddOrOverflow(sect->addr, sect->size, seg->vmaddr + seg->filesize);
                callback(seg->segname, segIndex, seg->vmaddr, sectName, sect->flags, sect->addr, sect->size, sect->align, sect->reserved1, sect->reserved2, illegalSectionSize, stop);
            }
            ++segIndex;
        }
        else if ( cmd->cmd == LC_SEGMENT ) {
            const segment_command* seg = (segment_command*)cmd;
            const section* const sectionsStart = (section*)((char*)seg + sizeof(struct segment_command));
            const section* const sectionsEnd   = &sectionsStart[seg->nsects];
            for (const section* sect=sectionsStart; !stop && (sect < sectionsEnd); ++sect) {
                const char* sectName = sect->sectname;
                char sectNameCopy[20];
                if ( sectName[15] != '\0' ) {
                    strlcpy(sectNameCopy, sectName, 17);
                    sectName = sectNameCopy;
                }
                bool illegalSectionSize = (sect->addr < seg->vmaddr) || greaterThanAddOrOverflow(sect->addr, sect->size, seg->vmaddr + seg->filesize);
                callback(seg->segname, segIndex, seg->vmaddr, sectName, sect->flags, sect->addr, sect->size, sect->align, sect->reserved1, sect->reserved2, illegalSectionSize, stop);
            }
            ++segIndex;
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
}

void MachOParser::forEachGlobalSymbol(Diagnostics& diag, void (^callback)(const char* symbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop)) const
{
    LinkEditInfo leInfo;
    getLinkEditPointers(diag, leInfo);
    if ( diag.hasError() )
        return;
    
    const bool is64Bit = is64();
    if ( leInfo.symTab != nullptr ) {
        uint32_t globalsStartIndex = 0;
        uint32_t globalsCount      = leInfo.symTab->nsyms;
        if ( leInfo.dynSymTab != nullptr ) {
            globalsStartIndex = leInfo.dynSymTab->iextdefsym;
            globalsCount      = leInfo.dynSymTab->nextdefsym;
        }
        uint32_t               maxStringOffset  = leInfo.symTab->strsize;
        const char*            stringPool       =             (char*)getLinkEditContent(leInfo.layout, leInfo.symTab->stroff);
        const struct nlist*    symbols          = (struct nlist*)   (getLinkEditContent(leInfo.layout, leInfo.symTab->symoff));
        const struct nlist_64* symbols64        = (struct nlist_64*)(getLinkEditContent(leInfo.layout, leInfo.symTab->symoff));
        bool                   stop             = false;
        for (uint32_t i=0; (i < globalsCount) && !stop; ++i) {
            if ( is64Bit ) {
                const struct nlist_64& sym = symbols64[globalsStartIndex+i];
                if ( sym.n_un.n_strx > maxStringOffset )
                    continue;
                if ( (sym.n_type & N_EXT) && ((sym.n_type & N_TYPE) == N_SECT) && ((sym.n_type & N_STAB) == 0) )
                    callback(&stringPool[sym.n_un.n_strx], sym.n_value, sym.n_type, sym.n_sect, sym.n_desc, stop);
            }
            else {
                const struct nlist& sym = symbols[globalsStartIndex+i];
                if ( sym.n_un.n_strx > maxStringOffset )
                    continue;
                if ( (sym.n_type & N_EXT) && ((sym.n_type & N_TYPE) == N_SECT) && ((sym.n_type & N_STAB) == 0) )
                    callback(&stringPool[sym.n_un.n_strx], sym.n_value, sym.n_type, sym.n_sect, sym.n_desc, stop);
            }
        }
    }
}

void MachOParser::forEachLocalSymbol(Diagnostics& diag, void (^callback)(const char* symbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop)) const
{
    LinkEditInfo leInfo;
    getLinkEditPointers(diag, leInfo);
    if ( diag.hasError() )
        return;
    
    const bool is64Bit = is64();
    if ( leInfo.symTab != nullptr ) {
        uint32_t localsStartIndex = 0;
        uint32_t localsCount      = leInfo.symTab->nsyms;
        if ( leInfo.dynSymTab != nullptr ) {
            localsStartIndex = leInfo.dynSymTab->ilocalsym;
            localsCount      = leInfo.dynSymTab->nlocalsym;
        }
        uint32_t               maxStringOffset  = leInfo.symTab->strsize;
        const char*            stringPool       =             (char*)getLinkEditContent(leInfo.layout, leInfo.symTab->stroff);
        const struct nlist*    symbols          = (struct nlist*)   (getLinkEditContent(leInfo.layout, leInfo.symTab->symoff));
        const struct nlist_64* symbols64        = (struct nlist_64*)(getLinkEditContent(leInfo.layout, leInfo.symTab->symoff));
        bool                   stop             = false;
        for (uint32_t i=0; (i < localsCount) && !stop; ++i) {
            if ( is64Bit ) {
                const struct nlist_64& sym = symbols64[localsStartIndex+i];
                if ( sym.n_un.n_strx > maxStringOffset )
                    continue;
                if ( ((sym.n_type & N_EXT) == 0) && ((sym.n_type & N_TYPE) == N_SECT) && ((sym.n_type & N_STAB) == 0) )
                    callback(&stringPool[sym.n_un.n_strx], sym.n_value, sym.n_type, sym.n_sect, sym.n_desc, stop);
            }
            else {
                const struct nlist& sym = symbols[localsStartIndex+i];
                if ( sym.n_un.n_strx > maxStringOffset )
                    continue;
                if ( ((sym.n_type & N_EXT) == 0) && ((sym.n_type & N_TYPE) == N_SECT) && ((sym.n_type & N_STAB) == 0) )
                    callback(&stringPool[sym.n_un.n_strx], sym.n_value, sym.n_type, sym.n_sect, sym.n_desc, stop);
            }
        }
    }
}


bool MachOParser::findExportedSymbol(Diagnostics& diag, const char* symbolName, void* extra, FoundSymbol& foundInfo, DependentFinder findDependent) const
{
    LinkEditInfo leInfo;
    getLinkEditPointers(diag, leInfo);
    if ( diag.hasError() )
        return false;
    if ( leInfo.dyldInfo != nullptr ) {
        const uint8_t* trieStart    = getLinkEditContent(leInfo.layout, leInfo.dyldInfo->export_off);
        const uint8_t* trieEnd      = trieStart + leInfo.dyldInfo->export_size;
        const uint8_t* node         = trieWalk(diag, trieStart, trieEnd, symbolName);
        if ( node == nullptr ) {
            // symbol not exported from this image. Seach any re-exported dylibs
            __block unsigned        depIndex = 0;
            __block bool            foundInReExportedDylib = false;
            forEachDependentDylib(^(const char* loadPath, bool isWeak, bool isReExport, bool isUpward, uint32_t compatVersion, uint32_t curVersion, bool& stop) {
                if ( isReExport && findDependent ) {
                    const mach_header*  depMH;
                    void*               depExtra;
                    if ( findDependent(depIndex, loadPath, extra, &depMH, &depExtra) ) {
                        bool depInRawCache = inRawCache() && (depMH->flags & 0x80000000);
                        MachOParser dep(depMH, depInRawCache);
                        if ( dep.findExportedSymbol(diag, symbolName, depExtra, foundInfo, findDependent) ) {
                            stop = true;
                            foundInReExportedDylib = true;
                        }
                    }
                    else {
                        fprintf(stderr, "could not find re-exported dylib %s\n", loadPath);
                    }
                }
                ++depIndex;
            });
            return foundInReExportedDylib;
        }
        const uint8_t* p = node;
        const uint64_t flags = read_uleb128(diag, p, trieEnd);
        if ( flags & EXPORT_SYMBOL_FLAGS_REEXPORT ) {
            if ( !findDependent )
                return false;
            // re-export from another dylib, lookup there
            const uint64_t ordinal = read_uleb128(diag, p, trieEnd);
            const char* importedName = (char*)p;
            if ( importedName[0] == '\0' )
                importedName = symbolName;
            assert(ordinal >= 1);
            if (ordinal > dependentDylibCount()) {
                diag.error("ordinal %lld out of range for %s", ordinal, symbolName);
                return false;
            }
            uint32_t depIndex = (uint32_t)(ordinal-1);
            const mach_header*  depMH;
            void*               depExtra;
            if ( findDependent(depIndex, dependentDylibLoadPath(depIndex), extra, &depMH, &depExtra) ) {
                bool depInRawCache = inRawCache() && (depMH->flags & 0x80000000);
                MachOParser depParser(depMH, depInRawCache);
                return depParser.findExportedSymbol(diag, importedName, depExtra, foundInfo, findDependent);
            }
            else {
                diag.error("dependent dylib %lld not found for re-exported symbol %s", ordinal, symbolName);
                return false;
            }
        }
        foundInfo.kind               = FoundSymbol::Kind::headerOffset;
        foundInfo.isThreadLocal      = false;
        foundInfo.foundInDylib       = header();
        foundInfo.foundExtra         = extra;
        foundInfo.value              = read_uleb128(diag, p, trieEnd);
        foundInfo.resolverFuncOffset = 0;
        foundInfo.foundSymbolName    = symbolName;
        if ( diag.hasError() )
            return false;
        switch ( flags & EXPORT_SYMBOL_FLAGS_KIND_MASK ) {
            case EXPORT_SYMBOL_FLAGS_KIND_REGULAR:
                if ( flags & EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER ) {
                    foundInfo.kind = FoundSymbol::Kind::headerOffset;
                    foundInfo.resolverFuncOffset = (uint32_t)read_uleb128(diag, p, trieEnd);
                }
                else {
                    foundInfo.kind = FoundSymbol::Kind::headerOffset;
                }
                break;
            case EXPORT_SYMBOL_FLAGS_KIND_THREAD_LOCAL:
                foundInfo.isThreadLocal = true;
                break;
            case EXPORT_SYMBOL_FLAGS_KIND_ABSOLUTE:
                foundInfo.kind = FoundSymbol::Kind::absolute;
                break;
            default:
                diag.error("unsupported exported symbol kind. flags=%llu at node offset=0x%0lX", flags, (long)(node-trieStart));
                return false;
        }
        return true;
    }
    else {
        // this is an old binary (before macOS 10.6), scan the symbol table
        foundInfo.foundInDylib = nullptr;
        uint64_t baseAddress = preferredLoadAddress();
        forEachGlobalSymbol(diag, ^(const char* aSymbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop) {
            if ( strcmp(aSymbolName, symbolName) == 0 ) {
                foundInfo.kind               = FoundSymbol::Kind::headerOffset;
                foundInfo.isThreadLocal      = false;
                foundInfo.foundInDylib       = header();
                foundInfo.foundExtra         = extra;
                foundInfo.value              = n_value - baseAddress;
                foundInfo.resolverFuncOffset = 0;
                foundInfo.foundSymbolName    = symbolName;
                stop = true;
            }
        });
        return (foundInfo.foundInDylib != nullptr);
    }
}


void MachOParser::getLinkEditLoadCommands(Diagnostics& diag, LinkEditInfo& result) const
{
    result.dyldInfo       = nullptr;
    result.symTab         = nullptr;
    result.dynSymTab      = nullptr;
    result.splitSegInfo   = nullptr;
    result.functionStarts = nullptr;
    result.dataInCode     = nullptr;
    result.codeSig        = nullptr;
    __block bool hasUUID    = false;
    __block bool hasVersion = false;
    __block bool hasEncrypt = false;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        switch ( cmd->cmd ) {
            case LC_DYLD_INFO:
            case LC_DYLD_INFO_ONLY:
                if ( cmd->cmdsize != sizeof(dyld_info_command) )
                    diag.error("LC_DYLD_INFO load command size wrong");
                else if ( result.dyldInfo != nullptr )
                    diag.error("multiple LC_DYLD_INFO load commands");
                result.dyldInfo = (dyld_info_command*)cmd;
                break;
            case LC_SYMTAB:
                if ( cmd->cmdsize != sizeof(symtab_command) )
                    diag.error("LC_SYMTAB load command size wrong");
                else if ( result.symTab != nullptr )
                    diag.error("multiple LC_SYMTAB load commands");
                result.symTab = (symtab_command*)cmd;
                break;
            case LC_DYSYMTAB:
                if ( cmd->cmdsize != sizeof(dysymtab_command) )
                    diag.error("LC_DYSYMTAB load command size wrong");
                else if ( result.dynSymTab != nullptr )
                    diag.error("multiple LC_DYSYMTAB load commands");
                result.dynSymTab = (dysymtab_command*)cmd;
                break;
            case LC_SEGMENT_SPLIT_INFO:
                if ( cmd->cmdsize != sizeof(linkedit_data_command) )
                    diag.error("LC_SEGMENT_SPLIT_INFO load command size wrong");
                else if ( result.splitSegInfo != nullptr )
                    diag.error("multiple LC_SEGMENT_SPLIT_INFO load commands");
                result.splitSegInfo = (linkedit_data_command*)cmd;
                break;
            case LC_FUNCTION_STARTS:
                if ( cmd->cmdsize != sizeof(linkedit_data_command) )
                    diag.error("LC_FUNCTION_STARTS load command size wrong");
                else if ( result.functionStarts != nullptr )
                    diag.error("multiple LC_FUNCTION_STARTS load commands");
                result.functionStarts = (linkedit_data_command*)cmd;
                break;
            case LC_DATA_IN_CODE:
                if ( cmd->cmdsize != sizeof(linkedit_data_command) )
                    diag.error("LC_DATA_IN_CODE load command size wrong");
                else if ( result.dataInCode != nullptr )
                    diag.error("multiple LC_DATA_IN_CODE load commands");
                result.dataInCode = (linkedit_data_command*)cmd;
                break;
            case LC_CODE_SIGNATURE:
                if ( cmd->cmdsize != sizeof(linkedit_data_command) )
                    diag.error("LC_CODE_SIGNATURE load command size wrong");
                else if ( result.codeSig != nullptr )
                    diag.error("multiple LC_CODE_SIGNATURE load commands");
                result.codeSig = (linkedit_data_command*)cmd;
                break;
            case LC_UUID:
                if ( cmd->cmdsize != sizeof(uuid_command) )
                    diag.error("LC_UUID load command size wrong");
                else if ( hasUUID )
                    diag.error("multiple LC_UUID load commands");
                hasUUID = true;
                break;
            case LC_VERSION_MIN_IPHONEOS:
            case LC_VERSION_MIN_MACOSX:
            case LC_VERSION_MIN_TVOS:
            case LC_VERSION_MIN_WATCHOS:
                if ( cmd->cmdsize != sizeof(version_min_command) )
                    diag.error("LC_VERSION_* load command size wrong");
                else if ( hasVersion )
                    diag.error("multiple LC_VERSION_MIN_* load commands");
                hasVersion = true;
                break;
            case LC_BUILD_VERSION:
                if ( cmd->cmdsize != (sizeof(build_version_command) + ((build_version_command*)cmd)->ntools * sizeof(build_tool_version)) )
                    diag.error("LC_BUILD_VERSION load command size wrong");
                else if ( hasVersion )
                    diag.error("multiple LC_BUILD_VERSION load commands");
                hasVersion = true;
                break;
            case LC_ENCRYPTION_INFO:
                if ( cmd->cmdsize != sizeof(encryption_info_command) )
                    diag.error("LC_ENCRYPTION_INFO load command size wrong");
                else if ( hasEncrypt )
                    diag.error("multiple LC_ENCRYPTION_INFO load commands");
                else if ( is64() )
                    diag.error("LC_ENCRYPTION_INFO found in 64-bit mach-o");
                hasEncrypt = true;
                break;
            case LC_ENCRYPTION_INFO_64:
                if ( cmd->cmdsize != sizeof(encryption_info_command_64) )
                    diag.error("LC_ENCRYPTION_INFO_64 load command size wrong");
                else if ( hasEncrypt )
                    diag.error("multiple LC_ENCRYPTION_INFO_64 load commands");
                else if ( !is64() )
                    diag.error("LC_ENCRYPTION_INFO_64 found in 32-bit mach-o");
                hasEncrypt = true;
                break;
        }
    });
    if ( diag.noError() && (result.dynSymTab != nullptr) && (result.symTab == nullptr) )
        diag.error("LC_DYSYMTAB but no LC_SYMTAB load command");
    
}

void MachOParser::getLinkEditPointers(Diagnostics& diag, LinkEditInfo& result) const
{
    getLinkEditLoadCommands(diag, result);
    if ( diag.noError() )
        getLayoutInfo(result.layout);
}

void MachOParser::forEachSegment(void (^callback)(const char* segName, uint32_t fileOffset, uint32_t fileSize, uint64_t vmAddr, uint64_t vmSize, uint8_t protections, bool& stop)) const
{
    Diagnostics diag;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        if ( cmd->cmd == LC_SEGMENT_64 ) {
            const segment_command_64* seg = (segment_command_64*)cmd;
            callback(seg->segname, (uint32_t)seg->fileoff, (uint32_t)seg->filesize, seg->vmaddr, seg->vmsize, seg->initprot, stop);
        }
        else if ( cmd->cmd == LC_SEGMENT ) {
            const segment_command* seg = (segment_command*)cmd;
            callback(seg->segname, seg->fileoff, seg->filesize, seg->vmaddr, seg->vmsize, seg->initprot, stop);
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
}

const uint8_t* MachOParser::trieWalk(Diagnostics& diag, const uint8_t* start, const uint8_t* end, const char* symbol)
{
    uint32_t visitedNodeOffsets[128];
    int visitedNodeOffsetCount = 0;
    visitedNodeOffsets[visitedNodeOffsetCount++] = 0;
    const uint8_t* p = start;
    while ( p < end ) {
        uint64_t terminalSize = *p++;
        if ( terminalSize > 127 ) {
            // except for re-export-with-rename, all terminal sizes fit in one byte
            --p;
            terminalSize = read_uleb128(diag, p, end);
            if ( diag.hasError() )
                return nullptr;
        }
        if ( (*symbol == '\0') && (terminalSize != 0) ) {
            return p;
        }
        const uint8_t* children = p + terminalSize;
        if ( children > end ) {
            diag.error("malformed trie node, terminalSize=0x%llX extends past end of trie\n", terminalSize);
            return nullptr;
        }
        uint8_t childrenRemaining = *children++;
        p = children;
        uint64_t nodeOffset = 0;
        for (; childrenRemaining > 0; --childrenRemaining) {
            const char* ss = symbol;
            bool wrongEdge = false;
            // scan whole edge to get to next edge
            // if edge is longer than target symbol name, don't read past end of symbol name
            char c = *p;
            while ( c != '\0' ) {
                if ( !wrongEdge ) {
                    if ( c != *ss )
                        wrongEdge = true;
                    ++ss;
                }
                ++p;
                c = *p;
            }
            if ( wrongEdge ) {
                // advance to next child
                ++p; // skip over zero terminator
                // skip over uleb128 until last byte is found
                while ( (*p & 0x80) != 0 )
                    ++p;
                ++p; // skip over last byte of uleb128
                if ( p > end ) {
                    diag.error("malformed trie node, child node extends past end of trie\n");
                    return nullptr;
                }
            }
            else {
                // the symbol so far matches this edge (child)
                // so advance to the child's node
                ++p;
                nodeOffset = read_uleb128(diag, p, end);
                if ( diag.hasError() )
                    return nullptr;
                if ( (nodeOffset == 0) || ( &start[nodeOffset] > end) ) {
                    diag.error("malformed trie child, nodeOffset=0x%llX out of range\n", nodeOffset);
                    return nullptr;
                }
                symbol = ss;
                break;
            }
        }
        if ( nodeOffset != 0 ) {
            if ( nodeOffset > (end-start) ) {
                diag.error("malformed trie child, nodeOffset=0x%llX out of range\n", nodeOffset);
                return nullptr;
            }
            for (int i=0; i < visitedNodeOffsetCount; ++i) {
                if ( visitedNodeOffsets[i] == nodeOffset ) {
                    diag.error("malformed trie child, cycle to nodeOffset=0x%llX\n", nodeOffset);
                    return nullptr;
                }
            }
            visitedNodeOffsets[visitedNodeOffsetCount++] = (uint32_t)nodeOffset;
            if ( visitedNodeOffsetCount >= 128 ) {
                diag.error("malformed trie too deep\n");
                return nullptr;
            }
            p = &start[nodeOffset];
        }
        else
            p = end;
    }
    return nullptr;
}


uint64_t MachOParser::read_uleb128(Diagnostics& diag, const uint8_t*& p, const uint8_t* end)
{
    uint64_t result = 0;
    int         bit = 0;
    do {
        if ( p == end ) {
            diag.error("malformed uleb128");
            break;
        }
        uint64_t slice = *p & 0x7f;
        
        if ( bit > 63 ) {
            diag.error("uleb128 too big for uint64");
            break;
        }
        else {
            result |= (slice << bit);
            bit += 7;
        }
    }
    while (*p++ & 0x80);
    return result;
}


int64_t MachOParser::read_sleb128(Diagnostics& diag, const uint8_t*& p, const uint8_t* end)
{
    int64_t  result = 0;
    int      bit = 0;
    uint8_t  byte = 0;
    do {
        if ( p == end ) {
            diag.error("malformed sleb128");
            break;
        }
        byte = *p++;
        result |= (((int64_t)(byte & 0x7f)) << bit);
        bit += 7;
    } while (byte & 0x80);
    // sign extend negative numbers
    if ( (byte & 0x40) != 0 )
        result |= (-1LL) << bit;
    return result;
}

bool MachOParser::is64() const
{
#if DYLD_IN_PROCESS
    return (sizeof(void*) == 8);
#else
    return (header()->magic == MH_MAGIC_64);
#endif
}




bool MachOParser::findClosestSymbol(uint64_t targetUnslidAddress, const char** symbolName, uint64_t* symbolUnslidAddr) const
{
    Diagnostics diag;
    __block uint64_t    closestNValueSoFar = 0;
    __block const char* closestNameSoFar   = nullptr;
    forEachGlobalSymbol(diag, ^(const char* aSymbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop) {
        if ( n_value <= targetUnslidAddress ) {
            if ( (closestNameSoFar == nullptr) || (closestNValueSoFar < n_value) ) {
                closestNValueSoFar = n_value;
                closestNameSoFar   = aSymbolName;
            }
        }
    });
    forEachLocalSymbol(diag, ^(const char* aSymbolName, uint64_t n_value, uint8_t n_type, uint8_t n_sect, uint16_t n_desc, bool& stop) {
        if ( n_value <= targetUnslidAddress ) {
            if ( (closestNameSoFar == nullptr) || (closestNValueSoFar < n_value) ) {
                closestNValueSoFar = n_value;
                closestNameSoFar   = aSymbolName;
            }
        }
    });
    if ( closestNameSoFar == nullptr ) {
        return false;
    }
    
    *symbolName       = closestNameSoFar;
    *symbolUnslidAddr = closestNValueSoFar;
    return true;
}


#if DYLD_IN_PROCESS

bool MachOParser::findClosestSymbol(const void* addr, const char** symbolName, const void** symbolAddress) const
{
    uint64_t slide = getSlide();
    uint64_t symbolUnslidAddr;
    if ( findClosestSymbol((uint64_t)addr - slide, symbolName, &symbolUnslidAddr) ) {
        *symbolAddress = (const void*)(long)(symbolUnslidAddr + slide);
        return true;
    }
    return false;
}

intptr_t MachOParser::getSlide() const
{
    Diagnostics diag;
    __block intptr_t slide = 0;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
#if __LP64__
        if ( cmd->cmd == LC_SEGMENT_64 ) {
            const segment_command_64* seg = (segment_command_64*)cmd;
            if ( strcmp(seg->segname, "__TEXT") == 0 ) {
                slide = ((uint64_t)header()) - seg->vmaddr;
                stop = true;
            }
        }
#else
        if ( cmd->cmd == LC_SEGMENT ) {
            const segment_command* seg = (segment_command*)cmd;
            if ( strcmp(seg->segname, "__TEXT") == 0 ) {
                slide = ((uint32_t)header()) - seg->vmaddr;
                stop = true;
            }
        }
#endif
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
    return slide;
}

// this is only used by dlsym() at runtime.  All other binding is done when the closure is built.
bool MachOParser::hasExportedSymbol(const char* symbolName, DependentFinder finder, void** result) const
{
    typedef void* (*ResolverFunc)(void);
    ResolverFunc resolver;
    Diagnostics diag;
    FoundSymbol foundInfo;
    if ( findExportedSymbol(diag, symbolName, (void*)header(), foundInfo, finder) ) {
        switch ( foundInfo.kind ) {
            case FoundSymbol::Kind::headerOffset:
                *result = (uint8_t*)foundInfo.foundInDylib + foundInfo.value;
                break;
            case FoundSymbol::Kind::absolute:
                *result = (void*)(long)foundInfo.value;
                break;
            case FoundSymbol::Kind::resolverOffset:
                // foundInfo.value contains "stub".
                // in dlsym() we want to call resolver function to get final function address
                resolver = (ResolverFunc)((uint8_t*)foundInfo.foundInDylib + foundInfo.resolverFuncOffset);
                *result = (*resolver)();
                break;
        }
        return true;
    }
    return false;
}

const char* MachOParser::segmentName(uint32_t targetSegIndex) const
{
    __block const char* result = nullptr;
    __block uint32_t segIndex  = 0;
    forEachSegment(^(const char* segName, uint32_t fileOffset, uint32_t fileSize, uint64_t vmAddr, uint64_t vmSize, uint8_t protections, bool& stop) {
        if ( segIndex == targetSegIndex ) {
            result = segName;
            stop = true;
        }
        ++segIndex;
    });
    return result;
}

#else

#endif  //  !DYLD_IN_PROCESS

    
bool MachOParser::isFairPlayEncrypted(uint32_t& textOffset, uint32_t& size)
{
    textOffset = 0;
    size = 0;
    Diagnostics diag;
    forEachLoadCommand(diag, ^(const load_command* cmd, bool& stop) {
        if ( (cmd->cmd == LC_ENCRYPTION_INFO) || (cmd->cmd == LC_ENCRYPTION_INFO_64) ) {
            const encryption_info_command* encCmd = (encryption_info_command*)cmd;
            if ( encCmd->cryptid == 1 ) {
                // Note: cryptid is 0 in just-built apps.  The iTunes App Store sets cryptid to 1
                textOffset = encCmd->cryptoff;
                size       = encCmd->cryptsize;
            }
            stop = true;
        }
    });
    diag.assertNoError();   // any malformations in the file should have been caught by earlier validate() call
    return (textOffset != 0);
}


} // END namespace NSMachO
