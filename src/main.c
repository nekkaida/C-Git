#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
// Remove OpenSSL include
// #include <openssl/sha.h>

// Simple SHA-1 implementation
typedef struct {
    unsigned int state[5];
    unsigned int count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform(unsigned int state[5], const unsigned char buffer[64]);
void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const unsigned char* data, unsigned int len);
void SHA1Final(unsigned char digest[20], SHA1_CTX* context);

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) \
    | (rol(block->l[i],8)&0x00FF00FF))

#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

typedef union {
    unsigned char c[64];
    unsigned int l[16];
} CHAR64LONG16;

void SHA1Transform(unsigned int state[5], const unsigned char buffer[64]) {
    unsigned int a, b, c, d, e;
    CHAR64LONG16 block[1];

    memcpy(block, buffer, 64);

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void SHA1Init(SHA1_CTX* context) {
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}

void SHA1Update(SHA1_CTX* context, const unsigned char* data, unsigned int len) {
    unsigned int i, j;

    j = (context->count[0] >> 3) & 63;
    if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
    context->count[1] += (len >> 29);
    if ((j + len) > 63) {
        memcpy(&context->buffer[j], data, (i = 64-j));
        SHA1Transform(context->state, context->buffer);
        for ( ; i + 63 < len; i += 64) {
            SHA1Transform(context->state, data + i);
        }
        j = 0;
    }
    else i = 0;
    memcpy(&context->buffer[j], &data[i], len - i);
}

void SHA1Final(unsigned char digest[20], SHA1_CTX* context) {
    unsigned int i;
    unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);
    }
    SHA1Update(context, (unsigned char *)"\200", 1);
    while ((context->count[0] & 504) != 448) {
        SHA1Update(context, (unsigned char *)"\0", 1);
    }
    SHA1Update(context, finalcount, 8);
    for (i = 0; i < 20; i++) {
        digest[i] = (unsigned char)
         ((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
}

// Function to convert binary hash to hex string
void hash_to_hex(unsigned char *hash, char *hex) {
    const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 20; i++) {
        hex[i * 2] = hex_chars[(hash[i] >> 4) & 0xF];
        hex[i * 2 + 1] = hex_chars[hash[i] & 0xF];
    }
    hex[40] = '\0';
}

// Function to handle the hash-object command
int cmd_hash_object(int argc, char *argv[]) {
    int write_object = 0;
    const char *filename = NULL;
    
    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0) {
            write_object = 1;
        } else {
            filename = argv[i];
        }
    }
    
    if (filename == NULL) {
        fprintf(stderr, "Usage: hash-object [-w] <file>\n");
        return 1;
    }
    
    // Read the file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return 1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for file content
    char *content = malloc(file_size);
    if (content == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
    // Read file content
    if (fread(content, 1, file_size, file) != file_size) {
        fprintf(stderr, "Failed to read file %s\n", filename);
        free(content);
        fclose(file);
        return 1;
    }
    fclose(file);
    
    // Create header: "blob <size>\0"
    char header[100];
    int header_len = snprintf(header, sizeof(header), "blob %ld", file_size);
    header[header_len] = '\0'; // Add null byte
    header_len++; // Include null byte in length
    
    // Calculate SHA-1 hash
    SHA1_CTX sha_ctx;
    unsigned char hash[20]; // SHA-1 digest length is 20 bytes
    
    SHA1Init(&sha_ctx);
    SHA1Update(&sha_ctx, (unsigned char*)header, header_len);
    SHA1Update(&sha_ctx, (unsigned char*)content, file_size);
    SHA1Final(hash, &sha_ctx);
    
    // Convert hash to hex string
    char hash_hex[41];
    hash_to_hex(hash, hash_hex);
    
    // Print the hash
    printf("%s\n", hash_hex);
    
    // Rest of function unchanged...
    // Write object if -w flag is provided
    if (write_object) {
        // Create directories for the object
        char dir_path[100];
        snprintf(dir_path, sizeof(dir_path), ".git/objects/%c%c", hash_hex[0], hash_hex[1]);
        
        if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
            fprintf(stderr, "Failed to create directory %s: %s\n", dir_path, strerror(errno));
            free(content);
            return 1;
        }
        
        // Create the object file path
        char obj_path[150];
        snprintf(obj_path, sizeof(obj_path), "%s/%s", dir_path, hash_hex + 2);
        
        // Use system command to handle zlib compression
        char tmp_path[150];
        snprintf(tmp_path, sizeof(tmp_path), "/tmp/git_obj_%s", hash_hex);
        
        // Write header + content to temporary file
        FILE *tmp_file = fopen(tmp_path, "wb");
        if (tmp_file == NULL) {
            fprintf(stderr, "Failed to create temporary file: %s\n", strerror(errno));
            free(content);
            return 1;
        }
        
        // Write header and content
        fwrite(header, 1, header_len, tmp_file);
        fwrite(content, 1, file_size, tmp_file);
        fclose(tmp_file);
        
        // Compress using system command (multiple options for compatibility)
        int compression_success = 0;
        char cmd[512];
        
        // Try perl
        snprintf(cmd, sizeof(cmd), 
            "perl -MCompress::Zlib -e 'undef $/; open(F,\"%s\");$i=join(\"\",<F>);close(F); "
            "$c=compress($i);open(F,\">%s\");print F $c;close(F);' 2>/dev/null", 
            tmp_path, obj_path);
        
        if (system(cmd) == 0) {
            compression_success = 1;
        }
        
        // Try Python if perl failed
        if (!compression_success) {
            snprintf(cmd, sizeof(cmd), 
                "python3 -c 'import zlib, sys; "
                "data = open(\"%s\", \"rb\").read(); "
                "compressed = zlib.compress(data); "
                "open(\"%s\", \"wb\").write(compressed)' 2>/dev/null", 
                tmp_path, obj_path);
            
            if (system(cmd) == 0) {
                compression_success = 1;
            }
        }
        
        // Try Python2 if Python3 failed
        if (!compression_success) {
            snprintf(cmd, sizeof(cmd), 
                "python -c 'import zlib, sys; "
                "data = open(\"%s\", \"rb\").read(); "
                "compressed = zlib.compress(data); "
                "open(\"%s\", \"wb\").write(compressed)' 2>/dev/null", 
                tmp_path, obj_path);
            
            if (system(cmd) == 0) {
                compression_success = 1;
            }
        }
        
        // Clean up temporary file
        unlink(tmp_path);
        
        if (!compression_success) {
            fprintf(stderr, "Failed to compress and write object\n");
            free(content);
            return 1;
        }
    }
    
    free(content);
    return 0;
}

// Function to handle the cat-file command
int cmd_cat_file(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: cat-file [-p] <object>\n");
        return 1;
    }
    
    if (strcmp(argv[2], "-p") != 0) {
        fprintf(stderr, "Only -p option is supported\n");
        return 1;
    }
    
    const char *hash = argv[3];
    if (strlen(hash) != 40) {
        fprintf(stderr, "Invalid object hash\n");
        return 1;
    }
    
    // First 2 chars are directory name, rest is filename
    char path[1024];
    snprintf(path, sizeof(path), ".git/objects/%c%c/%s", hash[0], hash[1], hash + 2);
    
    // Check if file exists
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "Not a valid object name %s\n", hash);
        return 1;
    }
    
    // Try multiple approaches for decompression
    
    // 1. Try using perl (which should be available in most environments)
    char cmd1[2048];
    snprintf(cmd1, sizeof(cmd1), 
         "perl -MCompress::Zlib -e 'undef $/; $i = \"\"; "
         "open(F, \"%s\"); $c = join(\"\", <F>); close(F); "
         "$u = uncompress($c); $p = index($u, \"\\0\"); "
         "print substr($u, $p+1);' 2>/dev/null", path);
    
    int result1 = system(cmd1);
    if (result1 == 0) {
        return 0;  // Success!
    }
    
    // 2. Try using Python as a fallback
    char cmd2[2048];
    snprintf(cmd2, sizeof(cmd2), 
          "python3 -c 'import zlib, sys; "
          "data = open(\"%s\", \"rb\").read(); "
          "decompressed = zlib.decompress(data); "
          "null_index = decompressed.find(b\"\\0\"); "
          "if null_index != -1: "
          "    sys.stdout.buffer.write(decompressed[null_index+1:])' 2>/dev/null", path);
    
    int result2 = system(cmd2);
    if (result2 == 0) {
        return 0;  // Success!
    }
    
    // 3. Try using Python2 as another fallback
    char cmd3[2048];
    snprintf(cmd3, sizeof(cmd3), 
          "python -c 'import zlib, sys; "
          "data = open(\"%s\", \"rb\").read(); "
          "decompressed = zlib.decompress(data); "
          "null_index = decompressed.find(\"\\0\"); "
          "if null_index != -1: "
          "    sys.stdout.write(decompressed[null_index+1:])' 2>/dev/null", path);
    
    int result3 = system(cmd3);
    if (result3 == 0) {
        return 0;  // Success!
    }
    
    // 4. One more approach using Unix tools
    char cmd4[2048];
    snprintf(cmd4, sizeof(cmd4),
          "cat %s | { od -A n -t x1 -v | tr -d ' \\n'; } | "
          "xxd -r -p | gzip -d 2>/dev/null | "
          "sed -e '1s/^[^\\x0]*\\x0//' 2>/dev/null", path);
    
    int result4 = system(cmd4);
    if (result4 == 0) {
        return 0;  // Success!
    }
    
    fprintf(stderr, "Failed to decompress the object\n");
    return 1;
}

int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 2) {
        fprintf(stderr, "Usage: ./your_program.sh <command> [<args>]\n");
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "init") == 0) {
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");
        
        if (mkdir(".git", 0755) == -1 || 
            mkdir(".git/objects", 0755) == -1 || 
            mkdir(".git/refs", 0755) == -1) {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }
        
        FILE *headFile = fopen(".git/HEAD", "w");
        if (headFile == NULL) {
            fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
            return 1;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);
        
        printf("Initialized git directory\n");
    } else if (strcmp(command, "cat-file") == 0) {
        return cmd_cat_file(argc, argv);
    } else if (strcmp(command, "hash-object") == 0) {
        return cmd_hash_object(argc, argv);
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    
    return 0;
}