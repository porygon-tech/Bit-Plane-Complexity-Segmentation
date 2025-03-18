/* extract.c – compact BPCS extractor. Usage: extract stego.(ppm|png) out_secret.bin */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <jpeglib.h>
#include <png.h>
#define BS 8
#define MP 1
#define XP 5
#define TH 34
static unsigned char cb[BS*BS];
typedef struct { int w,h,m; unsigned char *d; } Img;
void init_cb(){ for(int i=0;i<BS*BS;i++){ int r=i/BS,c=i%BS; cb[i]=((r+c)&1)?1:0; } }
int getb(unsigned char v,int b){ return (v>>b)&1; }
unsigned char setb(unsigned char v,int b,int bit){ return bit?v|(1<<b):v&~(1<<b); }
int comp(const unsigned char *b, int n){ int t=0; for(int r=0;r<n;r++) for(int c=0;c<n-1;c++) t+= (b[r*n+c]!=b[r*n+c+1]); for(int c=0;c<n;c++) for(int r=0;r<n-1;r++) t+= (b[r*n+c]!=b[(r+1)*n+c]); return t; }
void conjugate(unsigned char *b, int n){ for(int i=0;i<n*n;i++) b[i]^=cb[i]; }
Img* rP(const char *f){ FILE *fp=fopen(f,"r"); if(!fp)return NULL; char fmt[3]; if(fscanf(fp,"%2s",fmt)!=1||strcmp(fmt,"P3")){ fclose(fp); return NULL; } int c; while((c=fgetc(fp))!='\n' && c!='#'); if(c=='#'){ while((c=fgetc(fp))!='\n' && c!=EOF); } Img *I=malloc(sizeof(Img)); if(fscanf(fp,"%d %d",&I->w,&I->h)!=2){ free(I); fclose(fp); return NULL; } fscanf(fp,"%d",&I->m); I->d=malloc(3*I->w*I->h); for(int i=0;i<3*I->w*I->h;i++) fscanf(fp,"%hhu",&I->d[i]); fclose(fp); return I; }
Img* rJ(const char *f){ FILE *fp=fopen(f,"rb"); if(!fp)return NULL; struct jpeg_decompress_struct cinfo; struct jpeg_error_mgr jerr; cinfo.err=jpeg_std_error(&jerr); jpeg_create_decompress(&cinfo); jpeg_stdio_src(&cinfo,fp); jpeg_read_header(&cinfo,TRUE); jpeg_start_decompress(&cinfo); Img *I=malloc(sizeof(Img)); I->w=cinfo.output_width; I->h=cinfo.output_height; I->m=255; int ps=cinfo.output_components; I->d=malloc(I->w*I->h*ps); while(cinfo.output_scanline<I->h){ unsigned char *r=I->d+cinfo.output_scanline*I->w*ps; jpeg_read_scanlines(&cinfo,&r,1); } jpeg_finish_decompress(&cinfo); jpeg_destroy_decompress(&cinfo); fclose(fp); return I; }
Img* rN(const char *f){ FILE *fp=fopen(f,"rb"); if(!fp)return NULL; png_structp p=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL); png_infop i=png_create_info_struct(p); if(setjmp(png_jmpbuf(p))){ png_destroy_read_struct(&p,&i,NULL); fclose(fp); return NULL; } png_init_io(p,fp); png_read_info(p,i); int w=png_get_image_width(p,i), h=png_get_image_height(p,i); png_byte ct=png_get_color_type(p,i), bd=png_get_bit_depth(p,i); if(bd==16) png_set_strip_16(p); if(ct==PNG_COLOR_TYPE_GRAY||ct==PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(p); if(ct&PNG_COLOR_MASK_ALPHA) png_set_strip_alpha(p); png_read_update_info(p,i); Img *I=malloc(sizeof(Img)); I->w=w; I->h=h; I->m=255; I->d=malloc(w*h*3); png_bytep *rws=malloc(sizeof(png_bytep)*h); for(int y=0;y<h;y++){ rws[y]=malloc(png_get_rowbytes(p,i)); } png_read_image(p,rws); for(int y=0;y<h;y++){ memcpy(I->d+y*w*3,rws[y],w*3); free(rws[y]); } free(rws); png_destroy_read_struct(&p,&i,NULL); fclose(fp); return I; }
Img* rI(const char *f){ const char *e=strrchr(f,'.'); if(e){ char l[10]; int i=0; while(e[i]&&i<9){ l[i]=tolower(e[i]); i++; } l[i]=0; if(!strcmp(l,".ppm")) return rP(f); if(!strcmp(l,".jpg")||!strcmp(l,".jpeg")) return rJ(f); if(!strcmp(l,".png")) return rN(f); } return rP(f); }
int wN(const char *f, const Img *I){ FILE *fp=fopen(f,"wb"); if(!fp)return 1; png_structp p=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL); png_infop i=png_create_info_struct(p); if(setjmp(png_jmpbuf(p))){ png_destroy_write_struct(&p,&i); fclose(fp); return 1; } png_init_io(p,fp); png_set_IHDR(p,i,I->w,I->h,8,PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT); png_write_info(p,i); int rb=I->w*3; png_bytep *rws=malloc(sizeof(png_bytep)*I->h); for(int y=0;y<I->h;y++) rws[y]=I->d+y*rb; png_write_image(p,rws); png_write_end(p,NULL); free(rws); png_destroy_write_struct(&p,&i); fclose(fp); return 0; }
void freeImg(Img *I){ if(I){ free(I->d); free(I); } }
void ext(Img *I, unsigned char **sOut, int *totOut){
 int cnt=0; int cap=(I->w/BS)*(I->h/BS)*(XP-MP+1)*63*3;
 unsigned char *S=calloc(cap,1);
 for(int ch=0; ch<3; ch++){
  for(int b=MP; b<=XP; b++){
   for(int y=0; y<=I->h-BS; y+=BS)
    for(int x=0; x<=I->w-BS; x+=BS){
     unsigned char blk[BS*BS]; for(int i=0,r=0; r<BS; r++) for(int c=0;c<BS;c++,i++){
      int p=((y+r)*I->w+(x+c))*3+ch; blk[i]=getb(I->d[p],b);
     }
     if(comp(blk,BS)<TH) continue;
     int fpix=((y+0)*I->w+(x+0))*3+ch; int f=getb(I->d[fpix],0);
     unsigned char sb[BS*BS]; int pos=1; for(int r=0;r<BS;r++) for(int c=0;c<BS;c++){
      if(r==0 && c==0) continue; sb[pos++]=blk[r*BS+c];
     }
     if(f){ unsigned char tmp[BS*BS]; tmp[0]=0; memcpy(&tmp[1],sb+1,BS*BS-1); conjugate(tmp,BS); memcpy(sb+1,tmp+1,BS*BS-1); }
     for(int i=1;i<BS*BS;i++) S[cnt++]=sb[i];
    }
  }
 }
 *sOut=S; *totOut=cnt;
}
int main(int argc, char *argv[]){
 if(argc!=3){ fprintf(stderr,"Usage: %s stego.(ppm|png) out_secret.bin\n",argv[0]); return 1; }
 init_cb();
 Img *I=rI(argv[1]); if(!I){ fprintf(stderr,"I/O err\n"); return 1; }
 unsigned char *S; int tot; ext(I,&S,&tot); freeImg(I);
 if(tot<32){ fprintf(stderr,"Not enough bits\n"); free(S); return 1; }
 int sz=0; for(int i=0;i<32;i++) sz=(sz<<1)|S[i];
 int exp=32+sz*8; if(tot<exp){ fprintf(stderr,"Bits %d < exp %d\n",tot,exp); free(S); return 1; }
 unsigned char *d=calloc(sz,1); int pos=32;
 for(int i=0;i<sz;i++){ unsigned char b=0; for(int j=0;j<8;j++) b=(b<<1)|S[pos++]; d[i]=b; }
 free(S);
 FILE *f=fopen(argv[2],"wb"); if(!f){ free(d); return 1; }
 fwrite(d,1,sz,f); fclose(f);
 printf("Extracted %d bytes\n",sz);
 free(d);
 return 0;
}
