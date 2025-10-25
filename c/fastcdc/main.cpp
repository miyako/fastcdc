//
//  main.cpp
//  fastcdc
//
//  Created by miyako on 2025/10/25.
//

#include "fastcdc.h"

static void usage(void)
{
    fprintf(stderr, "Usage:  fastcdc -r -i in -o out -\n\n");
    fprintf(stderr, "split text into content determined chunks\n\n");
    fprintf(stderr, " -%c path  : %s\n", 'i' , "document to parse");
    fprintf(stderr, " -%c path  : %s\n", 'o' , "text output (default=stdout)");
    fprintf(stderr, " %c        : %s\n", '-' , "use stdin for input");
    fprintf(stderr, " -%c       : %s\n", 'r' , "raw data (default=no)");
    fprintf(stderr, " -%c size  : %s\n", 'f' , "minimum size (default=2048)");
    fprintf(stderr, " -%c size  : %s\n", 't' , "maximum size (default=32768)");
    exit(1);
}

extern OPTARG_T optarg;
extern int optind, opterr, optopt;

#ifdef _WIN32
OPTARG_T optarg = 0;
int opterr = 1;
int optind = 1;
int optopt = 0;
int getopt(int argc, OPTARG_T *argv, OPTARG_T opts) {

    static int sp = 1;
    register int c;
    register OPTARG_T cp;
    
    if(sp == 1)
        if(optind >= argc ||
             argv[optind][0] != '-' || argv[optind][1] == '\0')
            return(EOF);
        else if(wcscmp(argv[optind], L"--") == NULL) {
            optind++;
            return(EOF);
        }
    optopt = c = argv[optind][sp];
    if(c == ':' || (cp=wcschr(opts, c)) == NULL) {
        ERR(L": illegal option -- ", c);
        if(argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return('?');
    }
    if(*++cp == ':') {
        if(argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            ERR(L": option requires an argument -- ", c);
            sp = 1;
            return('?');
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if(argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return(c);
}
#define ARGS (OPTARG_T)L"i:o:-rhcRnNf:t:"
#else
#define ARGS "i:o:-rhcRnNf:t:"
#endif

static void document_to_json(Document& document, std::string& text, bool printText) {
    
    Json::Value chunksNode(Json::arrayValue);
    for (const auto &chunk : document.chunks) {
        Json::Value chunkNode(Json::objectValue);
        chunkNode["start"] = chunk.offset;
        chunkNode["end"] = chunk.offset + chunk.length;
        if(printText) {
            chunkNode["text"] = chunk.text;
        }
        chunksNode.append(chunkNode);
    }
        
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    text = Json::writeString(writer, chunksNode);
}

int main(int argc, OPTARG_T argv[]) {
    
    const OPTARG_T input_path  = NULL;
    const OPTARG_T output_path = NULL;
    
    std::vector<uint8_t>src_data(0);
    
    int ch;
    std::string text;
    bool rawData = false;
    
    chunking = cdc_origin_64;
    int method = ORIGIN_CDC;
    uint32_t minSize = 8192 / 4;
    uint32_t maxSize = 8192 * 4;
    
    while ((ch = getopt(argc, argv, ARGS)) != -1){
        switch (ch){
            case 'i':
                input_path  = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case '-':
            {
                std::vector<uint8_t> buf(BUFLEN);
                size_t n;
                
                while ((n = fread(buf.data(), 1, buf.size(), stdin)) > 0) {
                    src_data.insert(src_data.end(), buf.begin(), buf.begin() + n);
                }
            }
                break;
            case 'r':
                rawData = true;
                break;
            case 'c':
                chunking = cdc_origin_64;
                method = ORIGIN_CDC;
                break;
            case 'R':
                chunking = rolling_data_2byes_64;
                method = ROLLING_2Bytes;
                break;
            case 'n':
                chunking = normalized_chunking_64;
                method = NORMALIZED_CDC;
                break;
            case 'N':
                chunking = normalized_chunking_2byes_64;
                method = NORMALIZED_2Bytes;
                break;
            case 'f':
            {
                OPTARG_T endptr = NULL;
                errno = 0;
                unsigned long tmp = STRTOUL(optarg, &endptr, 0);
                if ((errno == 0)
                    && (*endptr == '\0')
                    && (tmp <= UINT32_MAX)
                    && (tmp < maxSize)) {
                        minSize = (uint32_t)tmp;
                }
            }
                break;
            case 't':
            {
                OPTARG_T endptr = NULL;
                errno = 0;
                unsigned long tmp = STRTOUL(optarg, &endptr, 0);
                if ((errno == 0)
                    && (*endptr == '\0')
                    && (tmp <= UINT32_MAX)
                    && (tmp > minSize)) {
                        maxSize = (uint32_t)tmp;
                }
            }
                break;
            case 'h':
            default:
                usage();
                break;
        }
    }
    
    if((!src_data.size()) && (input_path != NULL)) {
        FILE *f = _fopen(input_path, _rb);
        if(f) {
            _fseek(f, 0, SEEK_END);
            size_t len = (size_t)_ftell(f);
            _fseek(f, 0, SEEK_SET);
            src_data.resize(len);
            fread(src_data.data(), 1, src_data.size(), f);
            fclose(f);
        }
    }
    
    if(!src_data.size()) {
        usage();
    }
    
    //start here
    
    Document document;
    switch (method) {
        case ROLLING_2Bytes:
            document.CDC = "ROLLING_2Bytes";
            break;
        case NORMALIZED_CDC:
            document.CDC = "NORMALIZED_CDC";
            break;
        case NORMALIZED_2Bytes:
            document.CDC = "NORMALIZED_2Bytes";
            break;
        default:
            document.CDC = "ORIGIN_CDC";
            break;
    }
    

    
    fastCDC_init(minSize, maxSize);

    int offset = 0, length = 0;
    unsigned char *fileCache = src_data.data();
    size_t CacheSize = src_data.size();
    size_t end = CacheSize - 1;
    
    for (;;) {

        Chunk chunk;
        chunk.offset = offset;
        length = (int)(CacheSize - offset + 1);
        length = chunking(fileCache + offset, length);
        if((offset + length) >= end) {
            chunk.length = length - 1;
        }else{
            chunk.length = length;
        }
        if(!rawData) {
            chunk.text = std::string(src_data.begin() + offset,
                                     src_data.begin() + offset + chunk.length);
        }
        document.chunks.push_back(chunk);
        offset += length;
        if (offset >= end)
            break;
    }
    
    document_to_json(document, text, !rawData);
    
    if(!output_path) {
        std::cout << text << std::endl;
    }else{
        FILE *f = _fopen(output_path, _wb);
        if(f) {
            fwrite(text.c_str(), 1, text.length(), f);
            fclose(f);
        }
    }
    
    return 0;
}

// functions
void fastCDC_init(uint32_t minSize, uint32_t maxSize) {
    
    unsigned char md5_digest[16];
    uint8_t seed[SeedLength];
    for (int i = 0; i < SymbolCount; i++) {

        for (int j = 0; j < SeedLength; j++) {
            seed[j] = i;
        }

        g_global_matrix[i] = 0;
        MD5(seed, SeedLength, md5_digest);
        memcpy(&(g_global_matrix[i]), md5_digest, 4);
        g_global_matrix_left[i] = g_global_matrix[i] << 1;
    }

    // 64 bit init
    for (int i = 0; i < SymbolCount; i++) {
        LEARv2[i] = GEARv2[i] << 1;
    }

    MinSize = minSize;
    MaxSize = maxSize;
    Mask_15 = 0xf9070353;  //  15个1
    Mask_11 = 0xd9000353;  //  11个1
    Mask_11_64 = 0x0000d90003530000;
    Mask_15_64 = 0x0000f90703530000;
    MinSize_divide_by_2 = MinSize / 2;
}

int normalized_chunking_64(unsigned char *p, int n) {
    uint64_t fingerprint = 0, digest;
    MinSize = 6 * 1024;
    int i = MinSize, Mid = 8 * 1024;

    // the minimal subChunk Size.
    if (n <= MinSize)
        return n;

    if (n > MaxSize)
        n = MaxSize;
    else if (n < Mid)
        Mid = n;

    while (i < Mid) {
        fingerprint = (fingerprint << 1) + (GEARv2[p[i]]);

        if ((!(fingerprint & FING_GEAR_32KB_64))) {
            return i;
        }

        i++;
    }

    while (i < n) {
        fingerprint = (fingerprint << 1) + (GEARv2[p[i]]);

        if ((!(fingerprint & FING_GEAR_02KB_64))) {
            return i;
        }

        i++;
    }

    return n;
}

int normalized_chunking_2byes_64(unsigned char *p, int n) {
    uint64_t fingerprint = 0, digest;
    MinSize = 6 * 1024;
    int i = MinSize / 2, Mid = 8 * 1024;

    // the minimal subChunk Size.
    if (n <= MinSize)
        return n;

    if (n > MaxSize)
        n = MaxSize;
    else if (n < Mid)
        Mid = n;

    while (i < Mid / 2) {
        int a = i * 2;
        fingerprint = (fingerprint << 2) + (LEARv2[p[a]]);

        if ((!(fingerprint & FING_GEAR_32KB_ls_64))) {
            return a;
        }

        fingerprint += GEARv2 [p[a + 1]];

        if ((!(fingerprint & FING_GEAR_32KB_64))) {
            return a + 1;
        }

        i++;
    }

    while (i < n / 2) {
        int a = i * 2;
        fingerprint = (fingerprint << 2) + (LEARv2[p[a]]);

        if ((!(fingerprint & FING_GEAR_02KB_ls_64))) {
            return a;
        }

        fingerprint += GEARv2[p[a + 1]];

        if ((!(fingerprint & FING_GEAR_02KB_64))) {
            return a + 1;
        }

        i++;
    }

    return n;
}

int rolling_data_2byes_64(unsigned char *p, int n) {
    uint64_t fingerprint = 0, digest;
    int i = MinSize_divide_by_2;

    // the minimal subChunk Size.
    if (n <= MinSize)
        return n;

    if (n > MaxSize) n = MaxSize;

    while (i < n / 2) {
        int a = i * 2;
        fingerprint = (fingerprint << 2) + (LEARv2[p[a]]);

        if ((!(fingerprint & FING_GEAR_08KB_ls_64))) {
            return a;
        }

        fingerprint += GEARv2[p[a + 1]];

        if ((!(fingerprint & FING_GEAR_08KB_64))) {
            return a + 1;
        }

        i++;
    }

    return n;
}

int cdc_origin_64(unsigned char *p, int n) {
    uint64_t fingerprint = 0, digest;
    int i = MinSize;
    // return n;
    // the minimal subChunk Size.
    if (n <= MinSize)
        return n;

    if (n > MaxSize) n = MaxSize;

    while (i < n) {
        fingerprint = (fingerprint << 1) + (GEARv2[p[i]]);
        if ((!(fingerprint & FING_GEAR_08KB_64))) {
            return i;
        }
        i++;
    }

    return n;
}
