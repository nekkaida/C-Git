#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

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
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    
    return 0;
}