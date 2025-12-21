#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <cJSON.h>
void usage(){
  printf("sec2bin: merge all section to one bin file\n");
  printf("-m  <on/off> : is merge a base binary file\n");
  printf("-p  <file name> : base binary file name\n");
  printf("-j  <json file> : section define json file\n");
  printf("-o  <output file> : output binary file name\n");
  printf("-s  <offset> : binary write address of flash, can be <B/K/M>, default is B, like 1024B\n");
  printf("-i  <file name> : input sections list,split with semicolon\n");
}
/*
 * header :
 * char magic[4] ;RTOS
 * int VERSION
 * int INDEX
 * int SECTION_COUNT
 * char SECTION_NAME[40]
 * int RAM ADDR (dex)
 * int RAM_SIZE (dex)
 * unsigned long long DISK ADDR
 * int DISK SIZE
 * int STACK SIZE
 */
#define MAGIC ('R' << 0 | 'T' << 8 | 'O' << 16 | 'S' << 24);
#define VERSION 1
#define HEADER_SIZE 2048
#pragma pack (1)
struct header_t {
  int magic;
  int version;
  int index;
  int section_count;
  char section_name[40];
  uint32_t ram_address;
  uint32_t ram_size;
  unsigned long long int disk_address;
  uint32_t disk_size;
  uint32_t stack_size;
  int reserved;
  fpos_t file_offset;
  unsigned long long file_size;
};

struct program_t {
  FILE *ifp;
  struct header_t *headers;
  int headers_count;
};
long get_file_size(FILE *stream) // 通过文件指针偏移获取文件大小
{
        long file_size = -1;
        long cur_offset = ftell(stream);	// 获取当前偏移位置
        if (cur_offset == -1) {
                printf("ftell failed :%s\n", strerror(errno));
                return -1;
        }
        if (fseek(stream, 0, SEEK_END) != 0) {	// 移动文件指针到文件末尾
                printf("fseek failed: %s\n", strerror(errno));
                return -1;
        }
        file_size = ftell(stream);	// 获取此时偏移值，即文件大小
        if (file_size == -1) {
                printf("ftell failed :%s\n", strerror(errno));
        }
        if (fseek(stream, cur_offset, SEEK_SET) != 0) {	// 将文件指针恢复初始位置
                printf("fseek failed: %s\n", strerror(errno));
                return -1;
        }
        return file_size;
}

int append_ips_to_program(struct program_t *prg, const char* fname,
                          int index, int total,
                          unsigned long long int *last_disk_addr, cJSON *json){


  if (!(fname && (prg->ifp = fopen(fname, "rb")))) {
    printf("*** open input section file %s fail\n", fname);
    return -1;
  }
  char *_t = strdup(fname), *_t2 = _t;
  char *_bt = basename(_t);
  if (!_bt)
    goto error;
  void* endp = _bt + strlen(_bt);
  char *app_name = strtok(_bt, ".");
  char *sec_type;
  if (!app_name)
    goto error;
  {
     char *remain = _bt + strlen(_bt) + 1;
     if ((intptr_t)remain >= (intptr_t)endp)
       goto error;
     sec_type = strrchr(remain, '.');
     if (!sec_type)
       goto error;
     sec_type[0] = '\0';
     sec_type = strrchr(remain, '.');
     if (!sec_type)
       goto error;
     sec_type = sec_type+1;
  }

  struct header_t *h = prg->headers;

  h->magic = MAGIC;
  h->version = VERSION;
  h->index = index;
  h->section_count = total;
  strncpy(h->section_name, app_name, 40);
  h->section_name[39] = '\0';
  {
    cJSON *section = cJSON_GetObjectItem(json, sec_type);
    if(!section) {
      printf("section name %s cannot find in json file\n", sec_type);
      goto error;
    }
    sscanf(cJSON_GetObjectItem(section,"ramaddr")->valuestring, "%x", &(h->ram_address));
    sscanf(cJSON_GetObjectItem(section,"ramsize")->valuestring, "%x", &(h->ram_size));
    sscanf(cJSON_GetObjectItem(section,"stacksize")->valuestring, "%x", &(h->stack_size));
  }
  h->disk_address = *last_disk_addr + HEADER_SIZE;
  h->file_size = get_file_size(prg->ifp);
  h->disk_size = (h->file_size + 511)/512 * 512;

  fgetpos(prg->ifp, &h->file_offset);
  *last_disk_addr = *last_disk_addr + HEADER_SIZE + h->disk_size;

  free(_t2);
  return 0;
error:
  free(_t2);
  fclose(prg->ifp);
  prg->ifp = NULL;
  printf("error\n");
  return -1;
}

int load_section_json(char * json_fname, cJSON **json) {
  FILE *fp = fopen(json_fname, "rb");
  if (!fp) {
    printf("open json file %s error\n", json_fname);
    return -1;
  }
  long size = get_file_size(fp);
  char *json_buf = (char*)malloc(size);
  int ret = fread(json_buf, 1, size, fp);
  if(ret != size) {
    printf("read json file %s error\n", json_fname);
    return -1;
  }
  *json = cJSON_Parse(json_buf);
  if (NULL == *json) {
    printf("load json file error\n");
    fclose(fp);
    free(json_buf);
    return -1;
  }
  //  printf("%s\n", cJSON_Print(*json));
  free(json_buf);
  return 0;
}

void dump_header(struct header_t *header){

    printf("magic: %s\n", (char*)&(header->magic));
    printf("version %d\n", header->version);
    printf("index %d\n", header->index);
    printf("count %d\n", header->section_count);
    printf("name %s\n", header->section_name);
    printf("ram addr 0x%x\n", header->ram_address);
    printf("ram size 0x%x\n", header->ram_size);
    printf("disk addr 0x%llx\n", header->disk_address);
    printf("disk size 0x%x\n", header->disk_size);
    printf("stack size 0x%x\n", header->stack_size);
    printf("file offset 0x%lx\n", *(off_t*)&(header->file_offset));
}

void write_binary_file(FILE *ofp, struct program_t *prg) {
  /* for () */
    int i, k;
  for (i = 0; i < prg->headers_count; i++) {
    struct header_t *h = &(prg->headers[i]);
    int hs = (int)((intptr_t)&(h->reserved) - (intptr_t)&(h->magic));
    fwrite(h, hs, 1, ofp);
    fseek(ofp, HEADER_SIZE - hs, SEEK_CUR);
    int fs = 0;

    char *buf = (char*)malloc(h->file_size);
    fsetpos(prg->ifp, &h->file_offset);
    /* for(int i =0; i< 10; i++) */
    /*   printf("get:[%d] %x\n", i, buf[i]); */

    int ret = fread(buf, 1, h->file_size, prg->ifp);
    if (ret != h->file_size){
      printf("Warning: %s may be read file error, ret %d, %lld\n", h->section_name,ret, h->file_size);
    }
    /*for(k =0; k < 20; k++) {
        printf("%x\n", buf[k]);
    }*/
    fwrite(buf, 1, ret, ofp);
    fseek(ofp, h->disk_size - ret, SEEK_CUR);
    free(buf);
  }
}
int read_programs(FILE *bfp, int *count, int *data, int size, struct header_t * header) {
  int ret = fread(data, size, 1, bfp);
  if (ret != 1) {
    printf("read header error %d\n", ret);
    return -1;
  }
  memcpy(header, data, sizeof(struct header_t)-4);
  int magic = ('R' << 0 | 'T' << 8 | 'O' << 16 | 'S' << 24);
  if(header->magic != magic){
    printf("magic error 0x%08x",header->magic);
    return -1;
  }
  if(header->version != 1){
    printf("version  %d error",header->version);
    return -1;
  }

  *count = header->section_count;
  if (*count <= 0) {
    printf("modules count errors\n");
    return -1;
  }

  fgetpos(bfp, &(header->file_offset));     // 获取当前文件位置
  fseek(bfp, header->disk_size, SEEK_CUR);  // 移动光标  ，相对于当前位置移动 header -> disk_size
  header->file_size = header->disk_size;    // 文件大小等于磁盘大小
  return header->index;
}
#define TRUE 1
#define FALSE 0
int main(int argc, char **argv)
{
  char opt;
  int do_merge = FALSE;
  char *base_binary_fname = NULL;
  char *sec_json_fname = NULL;
  char *out_binary_fname = NULL;
  unsigned long long int start_disk_address = 0, last_disk_addr =0;
  int ips_max_count = 512;
  char **input_secs = (char**) malloc(ips_max_count);
  int ips_count = 0;
  cJSON *json = NULL;
  int i;
  while((opt = getopt(argc, argv, "m:p:j:o:s:i:")) != -1){
    switch (opt){
      case 'm':{
        printf("opt: %c - %s\n", opt, optarg);
        if(!strcmp(optarg,  "on") || !strcmp(optarg,  "ON"))
          do_merge = TRUE;
        break;
      }
      case 'p': {
        printf("opt: %c - %s\n", opt, optarg);
        base_binary_fname = strdup(optarg);
        
        break;
      }
      case 'j': {
        printf("opt: %c - %s\n", opt, optarg);
        sec_json_fname = strdup(optarg);
       
        load_section_json(sec_json_fname, &json);
        break;
      }
      case 'o': {
        printf("opt: %c - %s\n", opt, optarg);
        out_binary_fname = strdup(optarg);
        break;
      }
      case 's': {
        printf("opt: %c - %s\n", opt, optarg);
        int len = strlen(optarg);
        if ( len < 2) {
          usage();
          return -1;
        }
        unsigned int s = atoi(optarg);
        switch(optarg[len-1]) {
          case 'M':
          case 'm':
            start_disk_address = s * 1024 * 1024;
            break;
          case 'k':
          case 'K':
            start_disk_address = s * 1024;
            break;
          case 'b':
          case 'B':
          default:
            start_disk_address = s;
            break;
        }
        last_disk_addr = start_disk_address;
        break;
      }
      case 'i': {
        printf("opt: %c - %s\n", opt, optarg);
        // split
        char *delims={ "; " };
        char *ifs = strdup(optarg);
        char *p = strtok(ifs, delims);

        //input_secs[ips_count++] = p;
        while(p != NULL){
          input_secs[ips_count++] = p;
          printf("p %s\n", p);
          if (ips_count >= ips_max_count) {
            char **tips = malloc(ips_max_count * 2);
            memcpy(tips, input_secs, ips_max_count);
            ips_max_count *= 2;
            free(input_secs);
            input_secs = tips;
          }
          p = strtok(NULL, delims);
        }
        break;
      }
      case 'h':
      default:
        usage();
        return 0;
    }
  }
  if(!json)
    return -1;
  const int header_size = 2048;
  int bs_count = 0;
  struct program_t bs_programs = {NULL, NULL};
  printf("open file %s \n", base_binary_fname);
  printf("---- do merge %d\n", do_merge);
  if (do_merge == TRUE) {
    FILE* bfp;
    if (base_binary_fname == NULL) {
      printf("Warning: enable base binary merge, but donot set -p parament\n");
    } else {
      if ((bfp = fopen(base_binary_fname, "rb")) == NULL) {
        printf("open file %s error\n", base_binary_fname);
        return -1;
      }
      struct header_t* headers = NULL;
      int *data = (int*)malloc(header_size);
      while(1) {
        struct header_t header;
        int n = 0;
        n = read_programs(bfp, &bs_count, data, header_size, &header);
        if (n < 0 || n > bs_count) {
          printf("**** load modules errors index %d(%d)\n", n, bs_count);
          return -1;
        }
        if (headers == NULL) {
          headers = (struct header_t *) calloc(bs_count, sizeof(struct header_t));
          if (headers == NULL) {
            printf("*** alloc memory errors\n");
            return -1;
          }
        }

        memcpy((void*)&headers[n], &header, sizeof(struct header_t));
        headers[n].section_count = bs_count + ips_count;  // bs_count 是指的是 原来的模块12块  然后新添加了3个模块 
        if(n + 1 >= bs_count) {
          last_disk_addr = headers[n].disk_address + headers[n].disk_size;
          break;
        }

      }
      bs_programs.ifp = bfp;
      bs_programs.headers = headers;
      bs_programs.headers_count = bs_count;
    }
  }

  if (bs_programs.headers != NULL){
    unsigned long long bs_disk_addr = bs_programs.headers[0].disk_address - header_size;
    if (start_disk_address != bs_disk_addr) {
      printf("base binary disk start address is 0x%llx, but setting 0x%llx, "
             "please Confirm and keep consistent\n",
             bs_disk_addr,
             start_disk_address);
      return -1;
    }
  }

  struct program_t ips_program[ips_count];
  for (i = 0; i < ips_count; i++) {
    ips_program[i].headers = (struct header_t*)malloc(sizeof(struct header_t)); // 给自定义添加的模块分配空间 
    append_ips_to_program(&ips_program[i], input_secs[i],
                          i + bs_count, bs_count + ips_count,
                          &last_disk_addr, json);
    ips_program[i].headers_count = 1;
  }

  FILE *ofp = fopen(out_binary_fname, "wb");
  char *sec_name_list[bs_count + ips_count];
  if (!ofp) {
    printf("open output file %s error\n", out_binary_fname);
    return -1;
  }
  write_binary_file(ofp, &bs_programs);

  for(i = 0; i< ips_count ; i++) {
    write_binary_file(ofp, &ips_program[i]);
  }
  fclose(ofp);
  printf("ending sec2binq2\n");
  return 0;
}
