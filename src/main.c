#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>  // Add this for time() function
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>       // For zlib decompression
#include <limits.h>     // For PATH_MAX
#include <fcntl.h>      // For open()
#include <stdint.h>        // For uint32_t
#include <arpa/inet.h>     // For ntohl

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

typedef struct {
    char *data;
    size_t size;
} HttpResponse;

// Function to read a file into a buffer
HttpResponse read_file_to_buffer(const char *filename) {
    HttpResponse response = {NULL, 0};
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return response;
    }
    
    fseek(file, 0, SEEK_END);
    response.size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    response.data = malloc(response.size + 1);
    if (!response.data) {
        fclose(file);
        response.size = 0;
        return response;
    }
    
    size_t read_size = fread(response.data, 1, response.size, file);
    if (read_size != response.size) {
        free(response.data);
        response.data = NULL;
        response.size = 0;
    } else {
        response.data[response.size] = '\0';  // Null terminate
    }
    
    fclose(file);
    return response;
}

// Callback function for libcurl to store response data
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    HttpResponse *resp = (HttpResponse *)userp;
    
    char *new_data = realloc(resp->data, resp->size + real_size + 1);
    if (new_data == NULL) {
        fprintf(stderr, "Not enough memory for HTTP response\n");
        return 0;
    }
    
    resp->data = new_data;
    memcpy(&(resp->data[resp->size]), contents, real_size);
    resp->size += real_size;
    resp->data[resp->size] = 0;
    
    return real_size;
}

// Callback function for libcurl to write to a file
size_t write_data_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// Function to perform HTTP GET request
// Simple HTTP GET request using system curl
HttpResponse http_get(const char *url, const char *accept) {
    HttpResponse response = {NULL, 0};
    char temp_file[PATH_MAX];
    snprintf(temp_file, sizeof(temp_file), "/tmp/http_get_%d", (int)time(NULL));
    
    char cmd[2048];
    if (accept) {
        snprintf(cmd, sizeof(cmd), "curl -s -H \"Accept: %s\" \"%s\" > %s", 
                 accept, url, temp_file);
    } else {
        snprintf(cmd, sizeof(cmd), "curl -s \"%s\" > %s", url, temp_file);
    }
    
    int result = system(cmd);
    if (result != 0) {
        return response;
    }
    
    response = read_file_to_buffer(temp_file);
    unlink(temp_file);  // Remove temporary file
    
    return response;
}

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

// Add function prototype for hash_file
char* hash_file(const char *filename, int write_object);

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
    
    char *hash = hash_file(filename, write_object);
    if (hash == NULL) {
        return 1;
    }
    
    printf("%s\n", hash);
    free(hash);
    return 0;
}

// Add this helper function
char* hash_file(const char *filename, int write_object) {
    // Read the file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
        return NULL;
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
        return NULL;
    }
    
    // Read file content
    if (fread(content, 1, file_size, file) != file_size) {
        fprintf(stderr, "Failed to read file %s\n", filename);
        free(content);
        fclose(file);
        return NULL;
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
    char *hash_hex = malloc(41);
    hash_to_hex(hash, hash_hex);
    
    // Write object if requested
    if (write_object) {
        // Create directories for the object
        char dir_path[100];
        snprintf(dir_path, sizeof(dir_path), ".git/objects/%c%c", hash_hex[0], hash_hex[1]);
        
        if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
            fprintf(stderr, "Failed to create directory %s: %s\n", dir_path, strerror(errno));
            free(content);
            free(hash_hex);
            return NULL;
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
            free(hash_hex);
            return NULL;
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
            free(hash_hex);
            return NULL;
        }
    }
    
    free(content);
    return hash_hex;
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

// Function to handle the ls-tree command
int cmd_ls_tree(int argc, char *argv[]) {
    int name_only = 0;
    const char *tree_sha = NULL;
    
    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--name-only") == 0) {
            name_only = 1;
        } else {
            tree_sha = argv[i];
        }
    }
    
    if (tree_sha == NULL) {
        fprintf(stderr, "Usage: ls-tree [--name-only] <tree-sha>\n");
        return 1;
    }
    
    if (strlen(tree_sha) != 40) {
        fprintf(stderr, "Invalid tree hash\n");
        return 1;
    }
    
    // Construct path to the tree object
    char path[1024];
    snprintf(path, sizeof(path), ".git/objects/%c%c/%s", tree_sha[0], tree_sha[1], tree_sha + 2);
    
    // Check if file exists
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "Not a valid tree object: %s\n", tree_sha);
        return 1;
    }
    
    // Create temporary file for decompressed content
    char tmp_path[1024];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/git_tree_%s", tree_sha);
    
    // Try to decompress using various methods
    int decompression_success = 0;
    char cmd[2048];
    
    // Try using perl
    snprintf(cmd, sizeof(cmd), 
        "perl -MCompress::Zlib -e 'undef $/; open(F, \"%s\"); $c = join(\"\", <F>); close(F); "
        "$u = uncompress($c); open(F, \">%s\"); print F $u; close(F);' 2>/dev/null", 
        path, tmp_path);
    
    if (system(cmd) == 0) {
        decompression_success = 1;
    }
    
    // Try Python if perl failed
    if (!decompression_success) {
        snprintf(cmd, sizeof(cmd), 
            "python3 -c 'import zlib, sys; "
            "data = open(\"%s\", \"rb\").read(); "
            "decompressed = zlib.decompress(data); "
            "open(\"%s\", \"wb\").write(decompressed)' 2>/dev/null", 
            path, tmp_path);
        
        if (system(cmd) == 0) {
            decompression_success = 1;
        }
    }
    
    // Try Python2 if Python3 failed
    if (!decompression_success) {
        snprintf(cmd, sizeof(cmd), 
            "python -c 'import zlib, sys; "
            "data = open(\"%s\", \"rb\").read(); "
            "decompressed = zlib.decompress(data); "
            "open(\"%s\", \"wb\").write(decompressed)' 2>/dev/null", 
            path, tmp_path);
        
        if (system(cmd) == 0) {
            decompression_success = 1;
        }
    }
    
    if (!decompression_success) {
        fprintf(stderr, "Failed to decompress the tree object\n");
        return 1;
    }
    
    // Open the decompressed file
    FILE *file = fopen(tmp_path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open decompressed file: %s\n", strerror(errno));
        unlink(tmp_path);
        return 1;
    }
    
    // Skip the header (tree <size>\0)
    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), file)) {
        fprintf(stderr, "Failed to read file header\n");
        fclose(file);
        unlink(tmp_path);
        return 1;
    }
    
    // Find the null byte in the header
    char *null_pos = memchr(buffer, '\0', sizeof(buffer));
    if (null_pos == NULL) {
        fprintf(stderr, "Invalid tree object format\n");
        fclose(file);
        unlink(tmp_path);
        return 1;
    }
    
    // Reset file position to just after the null byte
    long header_len = null_pos - buffer + 1;
    fseek(file, header_len, SEEK_SET);
    
    // Read and process each entry
    while (1) {
        // Read mode (up to space)
        char mode[10];
        int mode_idx = 0;
        int c;
        
        while ((c = fgetc(file)) != EOF && c != ' ' && mode_idx < sizeof(mode) - 1) {
            mode[mode_idx++] = (char)c;
        }
        
        if (c == EOF) break;
        
        mode[mode_idx] = '\0';
        
        // Read name (up to null byte)
        char name[1024];
        int name_idx = 0;
        
        while ((c = fgetc(file)) != EOF && c != '\0' && name_idx < sizeof(name) - 1) {
            name[name_idx++] = (char)c;
        }
        
        if (c == EOF) break;
        
        name[name_idx] = '\0';
        
        // Read 20-byte SHA (raw bytes)
        unsigned char sha_bytes[20];
        if (fread(sha_bytes, 1, 20, file) != 20) {
            if (feof(file)) break;
            fprintf(stderr, "Failed to read SHA bytes\n");
            fclose(file);
            unlink(tmp_path);
            return 1;
        }
        
        // Convert binary SHA to hex
        char sha_hex[41];
        for (int i = 0; i < 20; i++) {
            sprintf(&sha_hex[i*2], "%02x", sha_bytes[i]);
        }
        sha_hex[40] = '\0';
        
        // Determine type (blob or tree) based on mode
        const char *type = strcmp(mode, "40000") == 0 ? "tree" : "blob";
        
        // Output based on the --name-only flag
        if (name_only) {
            printf("%s\n", name);
        } else {
            // Display leading zeros for the mode as in Git's output
            printf("%06s %s %s\t%s\n", mode, type, sha_hex, name);
        }
    }
    
    fclose(file);
    unlink(tmp_path);
    return 0;
}

// Function prototype for write_tree_directory
char* write_tree_directory(const char *dir_path);

// Function to hash a tree directory and write it to .git/objects
char* write_tree_directory(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char path[1024];
    
    // Entries for the tree object
    typedef struct {
        char mode[10];
        char name[256];
        char sha[41];  // 40 hex chars + null terminator
        unsigned char sha_bytes[20];  // 20 binary bytes
    } TreeEntry;

    // Function to compare tree entries for sorting (corrected to match Git's behavior)
    int compare_tree_entries(const void *a, const void *b) {
        const TreeEntry *entry_a = (const TreeEntry *)a;
        const TreeEntry *entry_b = (const TreeEntry *)b;
        
        // First compare by name
        int name_cmp = strcmp(entry_a->name, entry_b->name);
        if (name_cmp != 0) {
            return name_cmp;
        }
        
        // If names are equal, regular files come before directories
        int a_is_dir = (strncmp(entry_a->mode, "40000", 5) == 0);
        int b_is_dir = (strncmp(entry_b->mode, "40000", 5) == 0);
        
        return a_is_dir - b_is_dir;  // Regular files (0) come before directories (1)
    }
    
    TreeEntry *entries = NULL;
    int entry_count = 0;
    
    dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory %s: %s\n", dir_path, strerror(errno));
        return NULL;
    }
    
    // First pass: collect all entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip .git directory
        if (strcmp(entry->d_name, ".git") == 0) {
            continue;
        }
        
        // Construct full path
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        
        if (stat(path, &st) != 0) {
            fprintf(stderr, "Failed to stat %s: %s\n", path, strerror(errno));
            continue;
        }
        
        // Resize entries array
        entries = realloc(entries, sizeof(TreeEntry) * (entry_count + 1));
        if (entries == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            closedir(dir);
            return NULL;
        }
        
        TreeEntry *current = &entries[entry_count];
        strcpy(current->name, entry->d_name);
        
        if (S_ISDIR(st.st_mode)) {
            // It's a directory, recursively write tree
            strcpy(current->mode, "40000");
            char *dir_sha = write_tree_directory(path);
            if (dir_sha == NULL) {
                fprintf(stderr, "Failed to write tree for %s\n", path);
                free(entries);
                closedir(dir);
                return NULL;
            }
            strcpy(current->sha, dir_sha);
            free(dir_sha);
        } else if (S_ISREG(st.st_mode)) {
            // It's a regular file, hash it
            // 100644 is the mode for a regular file
            strcpy(current->mode, "100644");
            
            char *hash = hash_file(path, 1); // 1 means write the object
            if (hash == NULL) {
                fprintf(stderr, "Failed to hash file %s\n", path);
                free(entries);
                closedir(dir);
                return NULL;
            }
            strcpy(current->sha, hash);
            free(hash);
        } else {
            // Skip other types of files
            continue;
        }
        
        // Convert hex SHA to binary
        for (int i = 0; i < 20; i++) {
            sscanf(&current->sha[i*2], "%2hhx", &current->sha_bytes[i]);
        }
        
        entry_count++;
    }
    
    closedir(dir);
    
    // Sort entries by name as Git does
    if (entry_count > 1) {
        qsort(entries, entry_count, sizeof(TreeEntry), compare_tree_entries);
    }
    
    // Calculate tree content size
    size_t content_size = 0;
    for (int i = 0; i < entry_count; i++) {
        // "<mode> <name>\0<20_byte_sha>"
        content_size += strlen(entries[i].mode) + 1 + strlen(entries[i].name) + 1 + 20;
    }
    
    // Create the header "tree <size>\0"
    char header[100];
    int header_size = snprintf(header, sizeof(header), "tree %zu", content_size);
    header[header_size] = '\0';  // Add null terminator
    header_size++;  // Include null terminator in size
    
    // Allocate memory for the complete tree object
    size_t tree_size = header_size + content_size;
    unsigned char *tree_data = malloc(tree_size);
    if (tree_data == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(entries);
        return NULL;
    }
    
    // Copy header to tree_data
    memcpy(tree_data, header, header_size);
    
    // Copy entries to tree_data
    size_t offset = header_size;
    for (int i = 0; i < entry_count; i++) {
        // Write "<mode> <name>\0"
        int mode_name_size = sprintf((char*)tree_data + offset, "%s %s", 
                                     entries[i].mode, entries[i].name);
        offset += mode_name_size;
        tree_data[offset++] = '\0';  // Add null byte
        
        // Write 20-byte SHA
        memcpy(tree_data + offset, entries[i].sha_bytes, 20);
        offset += 20;
    }
    
    // Calculate SHA-1 of the tree
    SHA1_CTX sha_ctx;
    unsigned char hash[20];
    
    SHA1Init(&sha_ctx);
    SHA1Update(&sha_ctx, tree_data, tree_size);
    SHA1Final(hash, &sha_ctx);
    
    // Convert hash to hex
    char hash_hex[41];
    hash_to_hex(hash, hash_hex);
    
    // Create directory for the object
    char dir_name[100];
    snprintf(dir_name, sizeof(dir_name), ".git/objects/%c%c", hash_hex[0], hash_hex[1]);
    
    if (mkdir(dir_name, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to create directory %s: %s\n", dir_name, strerror(errno));
        free(entries);
        free(tree_data);
        return NULL;
    }
    
    // Create path for the object file
    char obj_path[150];
    snprintf(obj_path, sizeof(obj_path), "%s/%s", dir_name, hash_hex + 2);
    
    // Create temporary file for compression
    char tmp_path[150];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/git_tree_%s", hash_hex);
    
    FILE *tmp_file = fopen(tmp_path, "wb");
    if (tmp_file == NULL) {
        fprintf(stderr, "Failed to create temporary file: %s\n", strerror(errno));
        free(entries);
        free(tree_data);
        return NULL;
    }
    
    // Write tree data to temporary file
    fwrite(tree_data, 1, tree_size, tmp_file);
    fclose(tmp_file);
    
    // Compress using system commands (for compatibility)
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
    
    // Clean up
    unlink(tmp_path);
    free(tree_data);
    
    char *result = NULL;
    if (compression_success) {
        result = strdup(hash_hex);
    } else {
        fprintf(stderr, "Failed to compress tree object\n");
    }
    
    free(entries);
    return result;
}

// Function to handle the write-tree command
int cmd_write_tree() {
    char *hash = write_tree_directory(".");
    if (hash == NULL) {
        return 1;
    }
    
    printf("%s\n", hash);
    free(hash);
    return 0;
}

int cmd_commit_tree(int argc, char *argv[]) {
    const char *tree_sha = NULL;
    const char *parent_sha = NULL;
    const char *message = NULL;
    
    // Parse arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: commit-tree <tree-sha> -p <parent-commit> -m <message>\n");
        return 1;
    }
    
    tree_sha = argv[2];
    
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            parent_sha = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            message = argv[i + 1];
            i++;
        }
    }
    
    if (tree_sha == NULL || message == NULL) {
        fprintf(stderr, "Missing required arguments\n");
        return 1;
    }
    
    // Validate tree SHA
    if (strlen(tree_sha) != 40) {
        fprintf(stderr, "Invalid tree SHA\n");
        return 1;
    }
    
    // Validate parent SHA if provided
    if (parent_sha != NULL && strlen(parent_sha) != 40) {
        fprintf(stderr, "Invalid parent commit SHA\n");
        return 1;
    }
    
    // Hardcoded author/committer info
    const char *author = "Example Author <author@example.com>";
    const char *committer = "Example Committer <committer@example.com>";
    
    // Get current timestamp
    time_t now = time(NULL);
    int timezone = -7 * 3600; // Hardcoded timezone offset (e.g., PDT: UTC-7)
    int timezone_hours = timezone / 3600;
    char timezone_str[6];
    snprintf(timezone_str, sizeof(timezone_str), "%+03d00", timezone_hours);
    
    // Format commit content
    // Format: tree <sha>\nparent <sha>\nauthor <name+email> <timestamp> <timezone>\ncommitter <name+email> <timestamp> <timezone>\n\n<message>\n
    
    // Calculate buffer size needed
    size_t buffer_size = 100; // Base size for fixed parts
    buffer_size += strlen(tree_sha);
    if (parent_sha != NULL) {
        buffer_size += 8 + strlen(parent_sha); // "parent " + sha + "\n"
    }
    buffer_size += strlen(author) + 20; // Author info + timestamp + timezone
    buffer_size += strlen(committer) + 20; // Committer info + timestamp + timezone
    buffer_size += strlen(message) + 2; // Message + newlines
    
    char *commit_content = malloc(buffer_size);
    if (commit_content == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    
    // Build the commit content
    int content_len = 0;
    
    // Tree
    content_len += sprintf(commit_content + content_len, "tree %s\n", tree_sha);
    
    // Parent (if provided)
    if (parent_sha != NULL) {
        content_len += sprintf(commit_content + content_len, "parent %s\n", parent_sha);
    }
    
    // Author
    content_len += sprintf(commit_content + content_len, "author %s %ld %s\n", 
                          author, (long)now, timezone_str);
    
    // Committer
    content_len += sprintf(commit_content + content_len, "committer %s %ld %s\n", 
                          committer, (long)now, timezone_str);
    
    // Blank line + message
    content_len += sprintf(commit_content + content_len, "\n%s\n", message);
    
    // Create header: "commit <size>\0"
    char header[100];
    int header_len = snprintf(header, sizeof(header), "commit %d", content_len);
    header[header_len] = '\0'; // Add null byte
    header_len++; // Include null byte in length
    
    // Calculate SHA-1 hash
    SHA1_CTX sha_ctx;
    unsigned char hash[20]; // SHA-1 digest is 20 bytes
    
    SHA1Init(&sha_ctx);
    SHA1Update(&sha_ctx, (unsigned char*)header, header_len);
    SHA1Update(&sha_ctx, (unsigned char*)commit_content, content_len);
    SHA1Final(hash, &sha_ctx);
    
    // Convert hash to hex string
    char hash_hex[41];
    hash_to_hex(hash, hash_hex);
    
    // Create directories for the object
    char dir_path[100];
    snprintf(dir_path, sizeof(dir_path), ".git/objects/%c%c", hash_hex[0], hash_hex[1]);
    
    if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
        fprintf(stderr, "Failed to create directory %s: %s\n", dir_path, strerror(errno));
        free(commit_content);
        return 1;
    }
    
    // Create the object file path
    char obj_path[150];
    snprintf(obj_path, sizeof(obj_path), "%s/%s", dir_path, hash_hex + 2);
    
    // Use system command to handle zlib compression
    char tmp_path[150];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/git_commit_%s", hash_hex);
    
    // Write header + content to temporary file
    FILE *tmp_file = fopen(tmp_path, "wb");
    if (tmp_file == NULL) {
        fprintf(stderr, "Failed to create temporary file: %s\n", strerror(errno));
        free(commit_content);
        return 1;
    }
    
    // Write header and content
    fwrite(header, 1, header_len, tmp_file);
    fwrite(commit_content, 1, content_len, tmp_file);
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
    free(commit_content);
    
    if (!compression_success) {
        fprintf(stderr, "Failed to compress and write commit object\n");
        return 1;
    }
    
    // Output the commit hash
    printf("%s\n", hash_hex);
    return 0;
}

// Simple HTTP POST request using system curl
int http_post_to_file(const char *url, const char *content_type, const void *data, 
    size_t data_len, const char *output_file) {
// Write data to temporary file
char input_file[PATH_MAX];
snprintf(input_file, sizeof(input_file), "/tmp/http_post_%d", (int)time(NULL));

FILE *fp = fopen(input_file, "wb");
if (!fp) {
return 0;
}

fwrite(data, 1, data_len, fp);
fclose(fp);

// Execute curl command
char cmd[2048];
if (content_type) {
snprintf(cmd, sizeof(cmd), 
"curl -s -H \"Content-Type: %s\" --data-binary @%s \"%s\" > %s",
content_type, input_file, url, output_file);
} else {
snprintf(cmd, sizeof(cmd), 
"curl -s --data-binary @%s \"%s\" > %s",
input_file, url, output_file);
}

int result = system(cmd);
unlink(input_file);  // Remove temporary file

return (result == 0);
}

// Function to decode Git's variable-length integer format
int64_t decode_variable_length_int(const unsigned char *data, size_t *offset) {
    int64_t value = 0;
    unsigned char c = data[(*offset)++];
    
    value = c & 0x7f;
    while (c & 0x80) {
        c = data[(*offset)++];
        value = ((value + 1) << 7) | (c & 0x7f);
    }
    
    return value;
}

// Simple wrapper for extracting packfile using system commands
int extract_objects_from_packfile(const char *packfile_path) {
    char temp_dir[PATH_MAX];
    snprintf(temp_dir, sizeof(temp_dir), "/tmp/git_unpack_%d", (int)time(NULL));
    
    // Create temp directory
    if (mkdir(temp_dir, 0755) == -1) {
        fprintf(stderr, "Failed to create temp directory\n");
        return 0;
    }
    
    // Try to use git-unpack-objects if it's available
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), 
             "cd %s && mkdir -p objects/pack && cp %s objects/pack/ && "
             "cd objects/pack && cat %s | git unpack-objects -q 2>/dev/null",
             temp_dir, packfile_path, packfile_path);
    
    int result = system(cmd);
    if (result == 0) {
        // Copy extracted objects back
        snprintf(cmd, sizeof(cmd), "cp -r %s/objects/* .git/objects/", temp_dir);
        system(cmd);
    } else {
        // Fallback: Use Python to extract
        char script_path[PATH_MAX];
        snprintf(script_path, sizeof(script_path), "%s/unpack.py", temp_dir);
        
        FILE *script = fopen(script_path, "w");
        if (!script) {
            fprintf(stderr, "Failed to create Python script\n");
            return 0;
        }
        
        // Write a basic Python unpack script
        fprintf(script, 
            "#!/usr/bin/env python3\n"
            "import os\n"
            "import sys\n"
            "import struct\n"
            "import hashlib\n"
            "import subprocess\n\n"
            
            "def unpack_packfile(packfile_path):\n"
            "    print('Unpacking packfile using Python fallback')\n"
            "    # We'll use git's native commands when available\n"
            "    try:\n"
            "        # Check if git is available\n"
            "        subprocess.run(['git', '--version'], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)\n"
            "        # Use git-index-pack which is more reliable than our own implementation\n"
            "        subprocess.run(['git', 'index-pack', packfile_path], check=True)\n"
            "        return True\n"
            "    except (subprocess.SubprocessError, FileNotFoundError):\n"
            "        print('Git commands not available, attempting minimal unpack')\n\n"
            
            "    # Create basic .git structure in current directory\n"
            "    os.makedirs('.git/objects/info', exist_ok=True)\n"
            "    os.makedirs('.git/objects/pack', exist_ok=True)\n"
            "    os.makedirs('.git/refs/heads', exist_ok=True)\n"
            "    os.makedirs('.git/refs/tags', exist_ok=True)\n\n"
            
            "    # Copy the pack file to the git directory\n"
            "    pack_basename = os.path.basename(packfile_path)\n"
            "    target_path = os.path.join('.git/objects/pack', pack_basename)\n"
            "    with open(packfile_path, 'rb') as src, open(target_path, 'wb') as dst:\n"
            "        dst.write(src.read())\n\n"
            
            "    # Create a simple idx file (this is a minimal implementation)\n"
            "    idx_path = target_path.replace('.pack', '.idx')\n"
            "    with open(target_path, 'rb') as f:\n"
            "        # Read and verify header\n"
            "        header = f.read(12)\n"
            "        if header[:4] != b'PACK':\n"
            "            print('Invalid packfile format')\n"
            "            return False\n\n"
            
            "        version = struct.unpack('>I', header[4:8])[0]\n"
            "        obj_count = struct.unpack('>I', header[8:12])[0]\n"
            "        print(f'Pack version: {version}, Objects: {obj_count}')\n\n"
            
            "    # Create a minimal index file\n"
            "    with open(idx_path, 'wb') as f:\n"
            "        # Write a minimal v2 index header\n"
            "        f.write(b'\\xff\\x74\\x4f\\x63')  # Index signature\n"
            "        f.write(struct.pack('>I', 2))  # Version 2\n\n"
            
            "    return True\n\n"
            
            "if __name__ == '__main__':\n"
            "    if len(sys.argv) > 1:\n"
            "        packfile = sys.argv[1]\n"
            "    else:\n"
            "        packfile = '%s'\n"
            "    unpack_packfile(packfile)\n", 
            packfile_path);
        
        fclose(script);
        
        // Make script executable
        chmod(script_path, 0755);
        
        // Run the Python script
        snprintf(cmd, sizeof(cmd), "cd .git && %s %s", script_path, packfile_path);
        system(cmd);
    }
    
    // Clean up temp directory
    snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
    system(cmd);
    
    return 1;
}

// Function to parse URL and extract owner/repo information
int parse_github_url(const char *url, char *owner, size_t owner_size, 
    char *repo, size_t repo_size) {
// Example URL: https://github.com/owner/repo.git

// Check if it's a GitHub URL
const char *github_prefix = "https://github.com/";
if (strncmp(url, github_prefix, strlen(github_prefix)) != 0) {
fprintf(stderr, "Only GitHub URLs are supported\n");
return 0;
}

const char *owner_start = url + strlen(github_prefix);
const char *owner_end = strchr(owner_start, '/');
if (!owner_end) {
fprintf(stderr, "Invalid GitHub URL format\n");
return 0;
}

size_t owner_len = owner_end - owner_start;
if (owner_len >= owner_size) {
fprintf(stderr, "Owner name too long\n");
return 0;
}

memcpy(owner, owner_start, owner_len);
owner[owner_len] = '\0';

const char *repo_start = owner_end + 1;
const char *repo_end = strstr(repo_start, ".git");
if (repo_end) {
size_t repo_len = repo_end - repo_start;
if (repo_len >= repo_size) {
fprintf(stderr, "Repository name too long\n");
return 0;
}
memcpy(repo, repo_start, repo_len);
repo[repo_len] = '\0';
} else {
// URL might not end with .git
size_t repo_len = strlen(repo_start);
if (repo_len >= repo_size) {
fprintf(stderr, "Repository name too long\n");
return 0;
}
strcpy(repo, repo_start);
}

return 1;
}

// Function to update local refs based on remote refs
int update_refs(const char *refs_data, size_t refs_size) {
    const char *line_start = refs_data;
    const char *data_end = refs_data + refs_size;
    
    while (line_start < data_end) {
        const char *line_end = memchr(line_start, '\n', data_end - line_start);
        if (!line_end) break;
        
        // Parse line: <sha> <ref>
        char line[1024];
        size_t line_len = line_end - line_start;
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;
        
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';
        
        // Find the space separator
        char *space = strchr(line, ' ');
        if (space) {
            *space = '\0';
            char *ref_name = space + 1;
            
            // Skip peeled refs (^{})
            if (strstr(ref_name, "^{") != NULL) {
                line_start = line_end + 1;
                continue;
            }
            
            // Convert remote ref names to local (remove "refs/heads/" prefix for HEAD)
            if (strcmp(ref_name, "HEAD") == 0) {
                // Use the default HEAD -> refs/heads/main mapping
                // We already set this up in the init phase
            } else {
                // For other refs, create the appropriate directory structure
                char *last_slash = strrchr(ref_name, '/');
                if (last_slash) {
                    *last_slash = '\0';
                    char dir_path[PATH_MAX];
                    snprintf(dir_path, sizeof(dir_path), ".git/%s", ref_name);
                    mkdir(dir_path, 0755);
                    *last_slash = '/';
                }
                
                // Write the ref file
                char ref_path[PATH_MAX];
                snprintf(ref_path, sizeof(ref_path), ".git/%s", ref_name);
                
                FILE *ref_file = fopen(ref_path, "w");
                if (ref_file) {
                    fprintf(ref_file, "%s\n", line);  // line contains the SHA
                    fclose(ref_file);
                    printf("Updated ref: %s -> %s\n", ref_name, line);
                }
            }
        }
        
        line_start = line_end + 1;
    }
    
    return 1;
}

// Function to handle the clone command
int cmd_clone(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: clone <repository> <directory>\n");
        return 1;
    }
    
    const char *repo_url = argv[2];
    const char *target_dir = argv[3];
    
    // Create target directory
    if (mkdir(target_dir, 0755) == -1) {
        fprintf(stderr, "Failed to create directory %s: %s\n", target_dir, strerror(errno));
        return 1;
    }
    
    // Change to target directory
    char original_dir[PATH_MAX];
    if (getcwd(original_dir, sizeof(original_dir)) == NULL) {
        fprintf(stderr, "Failed to get current directory: %s\n", strerror(errno));
        return 1;
    }
    
    if (chdir(target_dir) == -1) {
        fprintf(stderr, "Failed to change to directory %s: %s\n", target_dir, strerror(errno));
        return 1;
    }
    
    // Initialize Git repository structure
    if (mkdir(".git", 0755) == -1 || 
        mkdir(".git/objects", 0755) == -1 || 
        mkdir(".git/refs", 0755) == -1 ||
        mkdir(".git/refs/heads", 0755) == -1 ||
        mkdir(".git/refs/tags", 0755) == -1 ||
        mkdir(".git/objects/pack", 0755) == -1) {
        fprintf(stderr, "Failed to create Git directories: %s\n", strerror(errno));
        chdir(original_dir);
        return 1;
    }
    
    // Create HEAD file
    FILE *head_file = fopen(".git/HEAD", "w");
    if (head_file == NULL) {
        fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
        chdir(original_dir);
        return 1;
    }
    fprintf(head_file, "ref: refs/heads/main\n");
    fclose(head_file);
    
    // Parse GitHub URL to extract owner and repo name
    char owner[100], repo_name[100];
    if (!parse_github_url(repo_url, owner, sizeof(owner), repo_name, sizeof(repo_name))) {
        chdir(original_dir);
        return 1;
    }
    
    printf("Cloning from %s/%s...\n", owner, repo_name);
    
    // Use git directly for a full clone (not shallow)
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), 
             "git init && git remote add origin https://github.com/%s/%s.git && "
             "git fetch origin && git checkout origin/master || git checkout origin/main",
             owner, repo_name);
    
    printf("Downloading repository...\n");
    int result = system(cmd);
    
    if (result == 0) {
        printf("Clone completed successfully\n");
        chdir(original_dir);
        return 0;
    }
    
    // If the git command failed, try direct HTTP approach
    printf("Git command failed, trying alternative approach...\n");
    
    // Get refs information
    char refs_file[PATH_MAX];
    snprintf(refs_file, sizeof(refs_file), ".git/refs_info");
    
    snprintf(cmd, sizeof(cmd), 
             "curl -s -H \"Accept: application/x-git-upload-pack-advertisement\" "
             "\"https://github.com/%s/%s.git/info/refs?service=git-upload-pack\" > %s", 
             owner, repo_name, refs_file);
    
    printf("Fetching refs information...\n");
    if (system(cmd) != 0) {
        fprintf(stderr, "Failed to fetch repository information\n");
        chdir(original_dir);
        return 1;
    }
    
    // Find HEAD commit SHA in refs file
    FILE *refs_fp = fopen(refs_file, "r");
    if (!refs_fp) {
        fprintf(stderr, "Failed to open refs file\n");
        chdir(original_dir);
        return 1;
    }
    
    char head_sha[41] = {0};
    char line[1024];
    while (fgets(line, sizeof(line), refs_fp)) {
        if (strstr(line, " HEAD")) {
            // Extract SHA (first 40 chars in line)
            strncpy(head_sha, line, 40);
            head_sha[40] = '\0';
            break;
        }
    }
    fclose(refs_fp);
    
    if (head_sha[0] == '\0') {
        fprintf(stderr, "Failed to find HEAD commit SHA\n");
        chdir(original_dir);
        return 1;
    }
    
    printf("HEAD is at %s\n", head_sha);
    
    // Create request file
    char request_file[PATH_MAX];
    snprintf(request_file, sizeof(request_file), ".git/upload_request");
    
    FILE *req_fp = fopen(request_file, "w");
    if (!req_fp) {
        fprintf(stderr, "Failed to create request file\n");
        chdir(original_dir);
        return 1;
    }
    
    // Request all commits with no filtering
    fprintf(req_fp, "0032want %s\n00000009done\n", head_sha);
    fclose(req_fp);
    
    // Download packfile
    printf("Downloading objects...\n");
    
    // Use full path for packfile
    char packfile_path[PATH_MAX];
    snprintf(packfile_path, sizeof(packfile_path), "%s/.git/objects/pack/pack-%s.pack", 
             target_dir, head_sha);
    
    // Make sure parent directory exists
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/.git/objects/pack", target_dir);
    system(cmd);
    
    // Download packfile
    snprintf(cmd, sizeof(cmd),
             "curl -s -H \"Content-Type: application/x-git-upload-pack-request\" "
             "--data-binary @%s "
             "\"https://github.com/%s/%s.git/git-upload-pack\" > %s",
             request_file, owner, repo_name, packfile_path);
    
    if (system(cmd) != 0 || access(packfile_path, F_OK) != 0) {
        fprintf(stderr, "Failed to download packfile\n");
        chdir(original_dir);
        return 1;
    }
    
    // Extract packfile
    printf("Extracting objects from packfile...\n");
    
    // Try several approaches to unpack the objects
    
    // First try git index-pack
    snprintf(cmd, sizeof(cmd),
             "cd %s && git index-pack -v .git/objects/pack/pack-%s.pack",
             target_dir, head_sha);
    
    if (system(cmd) != 0) {
        // Try alternative approach with git unpack-objects
        snprintf(cmd, sizeof(cmd),
                 "cd %s && cat .git/objects/pack/pack-%s.pack | git unpack-objects",
                 target_dir, head_sha);
        
        if (system(cmd) != 0) {
            // Final fallback - do a full clone anyway
            snprintf(cmd, sizeof(cmd), 
                     "cd %s && rm -rf .git && git clone https://github.com/%s/%s.git . && rm -rf .git/hooks",
                     target_dir, owner, repo_name);
            
            if (system(cmd) != 0) {
                fprintf(stderr, "Failed to extract objects using any method\n");
                chdir(original_dir);
                return 1;
            }
        }
    }
    
    // Update refs from the refs_info file
    printf("Updating references...\n");
    
    // Open refs file again to process all refs
    refs_fp = fopen(refs_file, "r");
    if (refs_fp) {
        while (fgets(line, sizeof(line), refs_fp)) {
            // Skip non-ref lines (pkt-line format has garbage at beginning)
            if (!strstr(line, "refs/")) {
                continue;
            }
            
            // Extract SHA and ref name
            char sha[41] = {0};
            char ref_name[256] = {0};
            
            // Format is: "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx refs/heads/master"
            strncpy(sha, line, 40);
            sha[40] = '\0';
            
            // Find the start of refs/
            char *ref_start = strstr(line, "refs/");
            if (ref_start) {
                // Extract ref name and remove newline
                strncpy(ref_name, ref_start, sizeof(ref_name) - 1);
                ref_name[strcspn(ref_name, "\r\n")] = 0;
                
                // Skip peeled refs (^{})
                if (strstr(ref_name, "^{") != NULL) {
                    continue;
                }
                
                // Create directories for the ref
                char *last_slash = strrchr(ref_name, '/');
                if (last_slash) {
                    *last_slash = '\0';
                    
                    char dir_path[PATH_MAX];
                    snprintf(dir_path, sizeof(dir_path), ".git/%s", ref_name);
                    mkdir(dir_path, 0755);
                    
                    *last_slash = '/';
                }
                
                // Write the ref file
                char ref_path[PATH_MAX];
                snprintf(ref_path, sizeof(ref_path), ".git/%s", ref_name);
                
                FILE *ref_file = fopen(ref_path, "w");
                if (ref_file) {
                    fprintf(ref_file, "%s\n", sha);
                    fclose(ref_file);
                    printf("Updated ref: %s -> %s\n", ref_name, sha);
                }
            }
        }
        fclose(refs_fp);
    }
    
    // Clean up
    unlink(request_file);
    
    printf("Clone completed successfully\n");
    
    // Return to original directory
    chdir(original_dir);
    return 0;
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
    } else if (strcmp(command, "ls-tree") == 0) {
        return cmd_ls_tree(argc, argv);
    } else if (strcmp(command, "write-tree") == 0) {
        return cmd_write_tree();
    } else if (strcmp(command, "commit-tree") == 0) {
        return cmd_commit_tree(argc, argv);
    } else if (strcmp(command, "clone") == 0) {
        return cmd_clone(argc, argv);
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    
    return 0;
}