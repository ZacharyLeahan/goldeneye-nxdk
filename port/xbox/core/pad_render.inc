/* Downstream jump pad drawn in the current player's world or hand pass. */
#include "dyn.h"
#include "floatvtx.h"
#include "finals_held.inc"

static void xboxPadWorldVertex(PortFloatVtx *v, coord3d at)
{
    Mtxf *proj=currentPlayerGetProjectionMatrixF();
    mtx4TransformVecInPlace(camGetWorldToScreenMtxf(), &at);
    float scale=D_800364CC;
    v->x=proj->m[0][0]*at.x*scale;
    v->y=proj->m[1][1]*at.y*scale;
    v->z=proj->m[2][2]*at.z*scale+proj->m[3][2];
    v->w=-at.z*scale;
}

static void xboxPadColor(PortFloatVtx *v, int r, int g, int b, int a)
{
    v->r=r; v->g=g; v->b=b; v->a=a;
}

// Convert the fork's generated tread and arrow to the texture format the
// cartridge renderer consumes. Both world and hand views sample this surface.
static u32 *xboxPadMaterial(void)
{
    // The software RSP reads executable-image textures as native GBI words.
    static u32 texture[32*32/2] __attribute__((aligned(8)));
    static int initialized;
    if (!initialized) {
        u8 pixels[32*32*3];
#include "finals_material.inc"
        for (int i=0;i<32*32;i++) {
            u16 color=GPACK_RGBA5551(pixels[i*3],pixels[i*3+1],pixels[i*3+2],255);
            if (i&1) texture[i/2]|=color;
            else texture[i/2]=(u32)color<<16;
        }
        initialized=1;
    }
    return texture;
}

static Gfx *xboxDrawPadMesh(Gfx *dl, const coord3d *center, int ghost, int valid)
{
    /* The three rings match the fork's rim, base and rubber launch surface. */
    PortFloatVtx *v=dynAllocate(24*sizeof(*v));
    memset(v,0,24*sizeof(*v));
    float hand[24][3];
    if (!center) {
        float w=getPlayer_c_screenwidth(), h=getPlayer_c_screenheight();
        float scale=fminf(w/1280.f,h/720.f);
        padHeldFace(hand,w/scale,h/scale,1);
    }
    for (int i=0;i<24;i++) {
        int ring=i/8;
        float theta=((i%8)+.5f)*.7853981634f;
        if (center) {
            float radius=PAD_RADIUS*(ring==0?1.f:ring==1?.97f:.72f);
            coord3d pos=*center;
            pos.x+=radius*cosf(theta);
            pos.z+=radius*sinf(theta);
            pos.y+=ring==0?8:ring==1?1:12;
            xboxPadWorldVertex(v+i,pos);
        } else {
            v[i].x=2*hand[i][0]-1;
            v[i].y=1-2*hand[i][1];
            v[i].w=1;
        }
        v[i].s=(.5f+.49f*cosf(theta))*32*32;
        v[i].t=(.5f+.49f*sinf(theta))*32*32;
        int light=(ring==0?112:ring==1?40:57)+(int)(13*cosf(theta));
        if (ghost) xboxPadColor(v+i,valid?95:225,valid?215:72,valid?86:59,ring==2?80:155);
        else xboxPadColor(v+i,light,light+7,light-14,255);
    }

    gDPPipeSync(dl++);
    gDPSetCycleType(dl++,G_CYC_1CYCLE);
    gDPSetTexturePersp(dl++,G_TP_PERSP);
    gDPSetAlphaCompare(dl++,G_AC_NONE);
    gSPClearGeometryMode(dl++,G_CULL_BOTH|G_FOG|G_LIGHTING|G_TEXTURE_GEN|G_ZBUFFER);
    gSPSetGeometryMode(dl++,G_SHADE|G_SHADING_SMOOTH);
    if (center) {
        gSPSetGeometryMode(dl++,G_ZBUFFER);
        if (ghost) {gDPSetRenderMode(dl++,G_RM_ZB_XLU_SURF,G_RM_ZB_XLU_SURF2);}
        else {gDPSetRenderMode(dl++,G_RM_ZB_OPA_SURF,G_RM_ZB_OPA_SURF2);}
    } else {gDPSetRenderMode(dl++,G_RM_OPA_SURF,G_RM_OPA_SURF2);}
    gDPSetCombineMode(dl++,G_CC_SHADE,G_CC_SHADE);
    gSPTexture(dl++,0xffff,0xffff,0,G_TX_RENDERTILE,G_ON);
    gSPFloatVertexExt(dl++,v,24,0);
    for (int i=0;i<8;i++) {
        int j=(i+1)%8;
        gSP2Triangles(dl++,i,j,j+8,0,i,j+8,i+8,0);
    }
    for (int i=0;i<8;i++) {
        int j=(i+1)%8;
        gSP2Triangles(dl++,i,j,j+16,0,i,j+16,i+16,0);
    }
    gDPPipeSync(dl++);
    gDPSetTextureLUT(dl++,G_TT_NONE);
    gDPSetTextureFilter(dl++,G_TF_BILERP);
    gDPLoadTextureBlock(dl++,xboxPadMaterial(),G_IM_FMT_RGBA,G_IM_SIZ_16b,32,32,0,
                       G_TX_CLAMP,G_TX_CLAMP,5,5,G_TX_NOLOD,G_TX_NOLOD);
    if (ghost) {
        gDPSetCombineLERP(dl++,1,SHADE,TEXEL0,SHADE,0,0,0,SHADE,
                              1,SHADE,TEXEL0,SHADE,0,0,0,SHADE);
    } else {gDPSetCombineMode(dl++,G_CC_DECALRGBA,G_CC_DECALRGBA);}
    for (int i=1;i<7;i++) {gSP1Triangle(dl++,16,16+i,17+i,0);}
    gSPTexture(dl++,0,0,0,G_TX_RENDERTILE,G_OFF);
    return dl;
}

Gfx *xboxDrawWorldPads(Gfx *dl)
{
    XboxLife *viewer=&xboxLives[get_cur_playernum()];
    for (int i=0;i<getPlayerCount();i++) {
        if (xboxLives[i].placed)
            dl=xboxDrawPadMesh(dl,&xboxLives[i].pad,0,1);
    }
    if (viewer->carrying && !viewer->wheelOpen && !g_CurrentPlayer->bonddead)
        dl=xboxDrawPadMesh(dl,&viewer->preview,1,viewer->validPreview);
    return dl;
}

static int xboxDrawHeldPad(Gfx **gdl)
{
    XboxLife *life=&xboxLives[get_cur_playernum()];
    if (!life->carrying || g_CurrentPlayer->bonddead) return 0;
    if (!life->wheelOpen && !g_CurrentPlayer->mpmenuon && !g_CurrentPlayer->watch_animation_state) {
        *gdl=xboxDrawPadMesh(*gdl,NULL,0,1);
    }
    return 1;
}
