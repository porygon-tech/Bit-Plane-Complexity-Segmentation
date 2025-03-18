/* extract_gui.c – Standalone GTK UI for BPCS extraction.
 * Usage: run extract_gui, drop stego files, choose output folder via “Browse…”,
 * and click “Extract” to extract secrets.
 */
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
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
int comp(const unsigned char *b,int n){ int t=0; for(int r=0;r<n;r++) for(int c=0;c<n-1;c++) t+=(b[r*n+c]!=b[r*n+c+1]); for(int c=0;c<n;c++) for(int r=0;r<n-1;r++) t+=(b[r*n+c]!=b[(r+1)*n+c]); return t; }
void conjugate(unsigned char *b,int n){ for(int i=0;i<n*n;i++) b[i]^=cb[i]; }
Img* rP(const char *f){ FILE *fp=fopen(f,"r"); if(!fp)return NULL; char fmt[3]; if(fscanf(fp,"%2s",fmt)!=1||strcmp(fmt,"P3")){ fclose(fp); return NULL; } int c; while((c=fgetc(fp))!='\n' && c!='#'); if(c=='#'){ while((c=fgetc(fp))!='\n' && c!=EOF); } Img *I=malloc(sizeof(Img)); if(fscanf(fp,"%d %d",&I->w,&I->h)!=2){ free(I); fclose(fp); return NULL; } fscanf(fp,"%d",&I->m); I->d=malloc(3*I->w*I->h); for(int i=0;i<3*I->w*I->h;i++) fscanf(fp,"%hhu",&I->d[i]); fclose(fp); return I; }
Img* rJ(const char *f){ FILE *fp=fopen(f,"rb"); if(!fp)return NULL; struct jpeg_decompress_struct cinfo; struct jpeg_error_mgr jerr; cinfo.err=jpeg_std_error(&jerr); jpeg_create_decompress(&cinfo); jpeg_stdio_src(&cinfo,fp); jpeg_read_header(&cinfo,TRUE); jpeg_start_decompress(&cinfo); Img *I=malloc(sizeof(Img)); I->w=cinfo.output_width; I->h=cinfo.output_height; I->m=255; int ps=cinfo.output_components; I->d=malloc(I->w*I->h*ps); while(cinfo.output_scanline<I->h){ unsigned char *r=I->d+cinfo.output_scanline*I->w*ps; jpeg_read_scanlines(&cinfo,&r,1); } jpeg_finish_decompress(&cinfo); jpeg_destroy_decompress(&cinfo); fclose(fp); return I; }
Img* rN(const char *f){ FILE *fp=fopen(f,"rb"); if(!fp)return NULL; png_structp p=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL); png_infop i=png_create_info_struct(p); if(setjmp(png_jmpbuf(p))){ png_destroy_read_struct(&p,&i,NULL); fclose(fp); return NULL; } png_init_io(p,fp); png_read_info(p,i); int w=png_get_image_width(p,i), h=png_get_image_height(p,i); png_byte ct=png_get_color_type(p,i), bd=png_get_bit_depth(p,i); if(bd==16) png_set_strip_16(p); if(ct==PNG_COLOR_TYPE_GRAY||ct==PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(p); if(ct&PNG_COLOR_MASK_ALPHA) png_set_strip_alpha(p); png_read_update_info(p,i); Img *I=malloc(sizeof(Img)); I->w=w; I->h=h; I->m=255; I->d=malloc(w*h*3); png_bytep *rws=malloc(sizeof(png_bytep)*h); for(int y=0;y<h;y++){ rws[y]=malloc(png_get_rowbytes(p,i)); } png_read_image(p,rws); for(int y=0;y<h;y++){ memcpy(I->d+y*w*3, rws[y], w*3); free(rws[y]); } free(rws); png_destroy_read_struct(&p,&i,NULL); fclose(fp); return I; }
Img* rI(const char *f){ const char *e=strrchr(f,'.'); if(e){ char l[10]; int i=0; while(e[i] && i<9){ l[i]=tolower(e[i]); i++; } l[i]=0; if(!strcmp(l,".ppm")) return rP(f); if(!strcmp(l,".jpg")||!strcmp(l,".jpeg")) return rJ(f); if(!strcmp(l,".png")) return rN(f); } return rP(f); }
void freeImg(Img *I){ if(I){ free(I->d); free(I); } }
int doExtraction(const char *infile, const char *outfile){
    Img *I=rI(infile); if(!I)return 1; int tot=0; int cap=(I->w/BS)*(I->h/BS)*(XP-MP+1)*63*3;
    unsigned char *S=calloc(cap,1);
    for(int ch=0; ch<3; ch++){
        for(int b=MP; b<=XP; b++){
            for(int y=0; y<=I->h-BS; y+=BS)
                for(int x=0; x<=I->w-BS; x+=BS){
                    unsigned char blk[BS*BS];
                    for(int r=0; r<BS; r++) for(int c=0; c<BS; c++){
                        int p=((y+r)*I->w+(x+c))*3+ch; blk[r*BS+c]=getb(I->d[p],b);
                    }
                    if(comp(blk,BS)<TH) continue;
                    int fp=((y+0)*I->w+(x+0))*3+ch; int flag=getb(I->d[fp],0);
                    unsigned char sb[BS*BS]; int pos=1;
                    for(int r=0; r<BS; r++) for(int c=0; c<BS; c++){ if(r==0 && c==0) continue; sb[pos++]=blk[r*BS+c]; }
                    if(flag){ unsigned char tmp[BS*BS]; tmp[0]=0; memcpy(tmp+1,sb+1,BS*BS-1); conjugate(tmp,BS); memcpy(sb+1,tmp+1,BS*BS-1); }
                    for(int i=1;i<BS*BS;i++) S[tot++]=sb[i];
                }
        }
    }
    if(tot<32){ free(S); freeImg(I); return 1; }
    int sz=0; for(int i=0;i<32;i++) sz=(sz<<1)|S[i];
    int exp=32+sz*8; if(tot<exp){ free(S); freeImg(I); return 1; }
    unsigned char *d=calloc(sz,1); int p=32;
    for(int i=0;i<sz;i++){ unsigned char b=0; for(int j=0;j<8;j++) b=(b<<1)|S[p++]; d[i]=b; }
    free(S);
    FILE *f=fopen(outfile,"wb"); if(!f){ free(d); freeImg(I); return 1; }
    fwrite(d,1,sz,f); fclose(f); free(d); freeImg(I);
    return 0;
}

/* GUI */
static GtkListStore *store;
static GList *files = NULL;
static GtkFileChooserButton *out_btn;
static void add_row(const char *in, const char *out){
    GtkTreeIter iter; gtk_list_store_append(store,&iter);
    gtk_list_store_set(store,&iter,0,in,1,out,-1);
}
static gboolean on_drag_data_received(GtkWidget *w, GdkDragContext *ctx, gint x, gint y,
                                        GtkSelectionData *data, guint info, guint time, gpointer u){
    char **uris=gtk_selection_data_get_uris(data);
    if(uris){
        for(int i=0; uris[i]!=NULL; i++){
            char *f=g_filename_from_uri(uris[i],NULL,NULL);
            if(f){ files = g_list_append(files,f); add_row(f,"Pending"); }
        }
        g_strfreev(uris);
    }
    gtk_drag_finish(ctx,TRUE,FALSE,time);
    return TRUE;
}
static void on_extract_clicked(GtkButton *btn, gpointer u){
    char *odir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(out_btn));
    if(!odir) odir=g_strdup(".");
    for(GList *l=files; l; l=l->next){
        char *in = (char*)l->data;
        char *base = g_path_get_basename(in);
        char *out = g_strdup_printf("%s/%s.secret",odir,base);
        g_free(base);
        if(doExtraction(in,out)==0){
            GtkTreeIter iter; gboolean valid=gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&iter);
            while(valid){
                gchar *s; gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,0,&s,-1);
                if(g_strcmp0(s,in)==0){ gtk_list_store_set(store,&iter,1,out,-1); g_free(s); break; }
                g_free(s); valid=gtk_tree_model_iter_next(GTK_TREE_MODEL(store),&iter);
            }
        }
        g_free(out);
    }
    g_free(odir);
}
int main(int argc, char *argv[]){
    gtk_init(&argc,&argv); init_cb();
    GtkWidget *win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "BPCS Extractor GUI");
    gtk_window_set_default_size(GTK_WINDOW(win),400,300);
    g_signal_connect(win,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    GtkWidget *vbox=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_container_add(GTK_CONTAINER(win),vbox);
    GtkWidget *drop=gtk_label_new("Drag stego files here");
    gtk_widget_set_size_request(drop,300,100);
    gtk_box_pack_start(GTK_BOX(vbox),drop,FALSE,FALSE,0);
    gtk_drag_dest_set(drop,GTK_DEST_DEFAULT_ALL,NULL,0,GDK_ACTION_COPY);
    GtkTargetEntry t[]={ {"text/uri-list",0,0} };
    gtk_drag_dest_set_target_list(drop,gtk_target_list_new(t,1));
    g_signal_connect(drop,"drag-data-received",G_CALLBACK(on_drag_data_received),NULL);
    GtkWidget *hbox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,0);
    GtkWidget *lbl=gtk_label_new("Output Dir:");
    gtk_box_pack_start(GTK_BOX(hbox),lbl,FALSE,FALSE,0);
    out_btn = GTK_FILE_CHOOSER_BUTTON(gtk_file_chooser_button_new("Browse...", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER));
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(out_btn),".");
    gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(out_btn),TRUE,TRUE,0);
    GtkWidget *btn=gtk_button_new_with_label("Extract");
    gtk_box_pack_start(GTK_BOX(vbox),btn,FALSE,FALSE,0);
    g_signal_connect(btn,"clicked",G_CALLBACK(on_extract_clicked),NULL);
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkCellRenderer *rend = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes("Stego File",rend,"text",0,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree),col);
    col = gtk_tree_view_column_new_with_attributes("Extracted File",rend,"text",1,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree),col);
    GtkWidget *sw = gtk_scrolled_window_new(NULL,NULL);
    gtk_container_add(GTK_CONTAINER(sw),tree);
    gtk_box_pack_start(GTK_BOX(vbox),sw,TRUE,TRUE,0);
    gtk_widget_show_all(win);
    gtk_main();
    return 0;
}
