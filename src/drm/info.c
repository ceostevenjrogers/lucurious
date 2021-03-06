/**
* Parts of this file contain functionality similar to what is in kms-quads device.c:
* https://gitlab.freedesktop.org/daniels/kms-quads/-/blob/master/kms.c
*/

/**
* The MIT License (MIT)
*
* Copyright (c) 2019-2020 Vincent Davis Jr.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#define LUCUR_DRM_API
#include <lucom.h>

#include <fcntl.h>

/* Can find here https://code.woboq.org/linux/linux/include/uapi/drm/drm_mode.h.html */
static const char *conn_types(uint32_t type) {
  switch (type) {
    case DRM_MODE_CONNECTOR_Unknown:     return "Unknown";
    case DRM_MODE_CONNECTOR_VGA:         return "VGA";
    case DRM_MODE_CONNECTOR_DVII:        return "DVI-I";
    case DRM_MODE_CONNECTOR_DVID:        return "DVI-D";
    case DRM_MODE_CONNECTOR_DVIA:        return "DVI-A";
    case DRM_MODE_CONNECTOR_Composite:   return "Composite";
    case DRM_MODE_CONNECTOR_SVIDEO:      return "SVIDEO";
    case DRM_MODE_CONNECTOR_LVDS:        return "LVDS";
    case DRM_MODE_CONNECTOR_Component:   return "Component";
    case DRM_MODE_CONNECTOR_9PinDIN:     return "DIN";
    case DRM_MODE_CONNECTOR_DisplayPort: return "DP";
    case DRM_MODE_CONNECTOR_HDMIA:       return "HDMI-A";
    case DRM_MODE_CONNECTOR_HDMIB:       return "HDMI-B";
    case DRM_MODE_CONNECTOR_TV:          return "TV";
    case DRM_MODE_CONNECTOR_eDP:         return "eDP";
    case DRM_MODE_CONNECTOR_VIRTUAL:     return "Virtual";
    case DRM_MODE_CONNECTOR_DSI:         return "DSI";
    default:                             return "Unknown";
  }
}

/* Intentionally did not add plane freeing into this function */
static void free_drm_objs(drmModeConnector **conn, drmModeEncoder **enc, drmModeCrtc **crtc, drmModePlane **plane) {
  if (conn) { // Just an extra check
    if (*conn) {
      drmModeFreeConnector(*conn);
      *conn = NULL;
    }
  }

  if (enc) { // Just an extra check
    if (*enc) {
      drmModeFreeEncoder(*enc);
      *enc = NULL;
    }
  }

  if (crtc) { // Just an extra check
    if (*crtc) {
      drmModeFreeCrtc(*crtc);
      *crtc = NULL;
    }
  }

  if (plane) { // Just an extra check
    if (*plane) {
      drmModeFreePlane(*plane);
      *plane = NULL;
    }
  }
}

void dlu_print_dconf_info(const char *device) {
  dlu_otma_mems ma = { .drmc_cnt = 1 };
  if (!dlu_otma(DLU_LARGE_BLOCK_PRIV, ma)) return;

  dlu_drm_core *core = dlu_drm_init_core();
  if (!core) goto exit_info;

  /* Exit if not in a tty */
  if (!dlu_drm_create_session(core)) { dlu_log_me(DLU_WARNING, "Please run command from within a TTY"); goto exit_info; }
  if (!dlu_drm_create_kms_node(core, device)) { dlu_log_me(DLU_WARNING, "Please run command from within a TTY"); goto exit_info; }

  drmModeRes *dmr = drmModeGetResources(core->device.kmsfd);
  if (!dmr) {
    dlu_print_msg(DLU_DANGER, "[x] drmModeGetResources: %s\n", strerror(errno));
    goto exit_info;
  }

  drmModePlaneRes *pres = drmModeGetPlaneResources(core->device.kmsfd);
  if (!pres) {
    dlu_print_msg(DLU_DANGER, "[x] drmModeGetPlaneResources: %s\n", strerror(errno));
    goto free_drm_res;
  }

  if (dmr->count_crtcs <= 0 || dmr->count_connectors <= 0 || dmr->count_encoders <= 0 || pres->count_planes <= 0) {
    dlu_print_msg(DLU_DANGER, "[x] '%s' is not a KMS node\n", device);
    goto free_plane_res;
  }

  drmModeConnector *conn = NULL; drmModeEncoder *enc = NULL;
  drmModeCrtc *crtc = NULL; drmModePlane *plane = NULL;
  uint32_t enc_crtc_id = 0, crtc_id = 0, fb_id = 0;
  uint64_t refresh = 0;

  for (int i = 0; i < dmr->count_connectors; i++) {
    conn = drmModeGetConnector(core->device.kmsfd, dmr->connectors[i]);
    if (!conn) {
      dlu_print_msg(DLU_DANGER, "[x] drmModeGetConnector: %s\n", strerror(errno));
      free_drm_objs(&conn, &enc, &crtc, &plane);
      goto free_plane_res;
    }

    /* Finding a encoder (a deprecated KMS object) for a given connector */
    for (int e = 0; e < dmr->count_encoders; e++) {
      if (dmr->encoders[e] == conn->encoder_id) {
        enc = drmModeGetEncoder(core->device.kmsfd, dmr->encoders[e]);
        if (!enc) {
          dlu_print_msg(DLU_DANGER, "[x] drmModeGetEncoder: %s\n", strerror(errno));
          free_drm_objs(&conn, &enc, &crtc, &plane);
          goto free_plane_res;
        }

        dlu_print_msg(DLU_SUCCESS, "\n\t\tConnector INFO\n");
        dlu_print_msg(DLU_INFO, "\tConn ID   : %u\tConn Index : %u\n", conn->connector_id, i);
        dlu_print_msg(DLU_INFO, "\tConn Type : %u\tConn Name  : %s\n", conn->connector_type, conn_types(conn->connector_type_id));
        dlu_print_msg(DLU_INFO, "\tEnc ID    : %u\n", conn->encoder_id);

        dlu_print_msg(DLU_SUCCESS, "\n\t\tEncoder INFO\n");
        dlu_print_msg(DLU_INFO, "\tEnc  ID   : %u\tEnc  Index : %u\n", enc->encoder_id, e);
        dlu_print_msg(DLU_INFO, "\tCrtc ID   : %u\n", enc->crtc_id); enc_crtc_id = enc->crtc_id;
        drmModeFreeEncoder(enc); enc = NULL;
        break;
      }
    }

    /* Finding a crtc for the given encoder */
    for (int c = 0; c < dmr->count_crtcs; c++) {
      if (dmr->crtcs[c] == enc_crtc_id) {
        crtc = drmModeGetCrtc(core->device.kmsfd, dmr->crtcs[c]);
        if (!crtc) {
          dlu_print_msg(DLU_DANGER, "[x] drmModeGetCrtc: %s\n", strerror(errno));
          free_drm_objs(&conn, &enc, &crtc, &plane);
          goto free_plane_res;
        }

        dlu_print_msg(DLU_SUCCESS, "\n\t\tCRTC INFO\n");
        dlu_print_msg(DLU_INFO, "\tCrtc ID   : %u\tCTRC Index : %u\n", crtc->crtc_id, c);
        dlu_print_msg(DLU_INFO, "\tFB ID     : %u\tmode valid : %u\n", crtc->buffer_id, crtc->mode_valid);
        dlu_print_msg(DLU_INFO, "\twidth     : %u\theight     : %u\n", crtc->width, crtc->height);
        /* DRM is supposed to provide a refresh interval, but often doesn't;
        * calculate our own in milliHz for higher precision anyway. */
        refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;
        crtc_id = crtc->crtc_id; fb_id = crtc->buffer_id;
        drmModeFreeCrtc(crtc); enc_crtc_id = UINT32_MAX;
        break;
      }
    }

    /* Only search for planes if a given CRTC has an encoder connected to it and a connector connected to that encoder */
    if (crtc) {
      for (uint32_t p = 0; p < pres->count_planes; p++) {
        plane = drmModeGetPlane(core->device.kmsfd, pres->planes[p]);
        if (!plane) {
          dlu_print_msg(DLU_DANGER, "[x] drmModeGetPlane: %s\n", strerror(errno));
          free_drm_objs(&conn, &enc, &crtc, &plane);
          goto free_plane_res;
        }

        /* look for primary plane for chosen crtc */
        if (plane->crtc_id == crtc_id && plane->fb_id == fb_id) {
          dlu_print_msg(DLU_SUCCESS, "\n\t\tPlane INFO\n");
          dlu_print_msg(DLU_INFO, "\tPLANE ID : %u\tPlane Index : %u\n", plane->plane_id, p);
          dlu_print_msg(DLU_INFO, "\tFB ID    : %u\tCRTC ID     : %u\n", plane->fb_id, plane->crtc_id);
          dlu_print_msg(DLU_INFO, "\tgamma sz : %u\tformats     : [", plane->gamma_size);
          for (uint32_t j = 0; j < plane->count_formats; j++)
            dlu_print_msg(DLU_INFO, "%u ", plane->formats[j]);
          dlu_print_msg(DLU_INFO, "]\n");
          dlu_print_msg(DLU_DANGER, "\n\tscreen refresh: %u\n", refresh); refresh = UINT64_MAX;
          dlu_print_msg(DLU_WARNING, "\n  Plane -> CRTC -> Encoder -> Connector Pair: %d\n", (i+1));
        }

        drmModeFreePlane(plane); plane = NULL;
      }

      /* Reset values */
      crtc = NULL; crtc_id = fb_id = 0;
    }

    drmModeFreeConnector(conn); conn = NULL;
  }

  fprintf(stdout, "\n");
free_plane_res:
  drmModeFreePlaneResources(pres);
free_drm_res:
  drmModeFreeResources(dmr);
exit_info:
  dlu_drm_freeup_core(core);
  dlu_release_blocks();
}

bool dlu_drm_q_output_dev_info(dlu_drm_core *core, dlu_drm_device_info *info) {
  bool ret = false; uint32_t cur_info = 0;

  if (core->device.kmsfd == UINT32_MAX) goto exit_func;

  drmModeRes *dmr = drmModeGetResources(core->device.kmsfd);
  if (!dmr) {
    dlu_log_me(DLU_DANGER, "[x] drmModeGetResources: %s\n", strerror(errno));
    goto exit_func;
  }

  drmModePlaneRes *pres = drmModeGetPlaneResources(core->device.kmsfd);
  if (!pres) {
    dlu_log_me(DLU_DANGER, "[x] drmModeGetPlaneResources: %s\n", strerror(errno));
    goto free_drm_res;
  }

  if (dmr->count_crtcs <= 0 || dmr->count_connectors <= 0 || dmr->count_encoders <= 0 || pres->count_planes <= 0) {
    dlu_log_me(DLU_DANGER, "[x] Current device is somehow not a KMS node");
    goto free_plane_res;
  }

  drmModeConnector *conn = NULL; drmModeEncoder *enc = NULL;
  drmModeCrtc *crtc = NULL; drmModePlane *plane = NULL;
  uint32_t enc_crtc_id = 0, crtc_id = 0, fb_id = 0;
  uint64_t refresh = 0;

  for (int i = 0; i < dmr->count_connectors; i++) {
    conn = drmModeGetConnector(core->device.kmsfd, dmr->connectors[i]);
    if (!conn) {
      dlu_log_me(DLU_DANGER, "[x] drmModeGetConnector: %s\n", strerror(errno));
      free_drm_objs(&conn, &enc, &crtc, &plane);
      goto free_plane_res;
    }

    /* Finding a encoder (a deprecated KMS object) for a given connector */
    for (int e = 0; e < dmr->count_encoders; e++) {
      if (dmr->encoders[e] == conn->encoder_id) {
        enc = drmModeGetEncoder(core->device.kmsfd, dmr->encoders[e]);
        if (!enc) {
          dlu_log_me(DLU_DANGER, "[x] drmModeGetEncoder: %s\n", strerror(errno));
          free_drm_objs(&conn, &enc, &crtc, &plane);
          goto free_plane_res;
        }

        info[cur_info].enc_idx = e;
        info[cur_info].conn_idx = i;
        snprintf(info[cur_info].conn_name, sizeof(info[cur_info].conn_name), "%s", conn_types(conn->connector_type_id));

        enc_crtc_id = enc->crtc_id;
        drmModeFreeEncoder(enc); enc = NULL;
        break;
      }
    }

    /* Finding a crtc for the given encoder */
    for (int c = 0; c < dmr->count_crtcs; c++) {
      if (dmr->crtcs[c] == enc_crtc_id) {
        crtc = drmModeGetCrtc(core->device.kmsfd, dmr->crtcs[c]);
        if (!crtc) {
          dlu_log_me(DLU_DANGER, "[x] drmModeGetCrtc: %s\n", strerror(errno));
          free_drm_objs(&conn, &enc, &crtc, &plane);
          goto free_plane_res;
        }
        
        info[cur_info].crtc_idx = c;
        refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;

        crtc_id = crtc->crtc_id; fb_id = crtc->buffer_id;
        drmModeFreeCrtc(crtc); enc_crtc_id = UINT32_MAX;
        break;
      }
    }

    /* Only search for planes if a given CRTC has an encoder connected to it and a connector connected to that encoder */
    if (crtc) {
      for (uint32_t p = 0; p < pres->count_planes; p++) {
        plane = drmModeGetPlane(core->device.kmsfd, pres->planes[p]);
        if (!plane) {
          dlu_log_me(DLU_DANGER, "[x] drmModeGetPlane: %s\n", strerror(errno));
          free_drm_objs(&conn, &enc, &crtc, &plane);
          goto free_plane_res;
        }

        /* look for primary plane for chosen crtc */
        if (plane->crtc_id == crtc_id && plane->fb_id == fb_id) {
           info[cur_info].plane_idx = p; info[cur_info].refresh  = refresh;
           cur_info++; ret = true; refresh = UINT64_MAX;
        }

        drmModeFreePlane(plane); plane = NULL;
      }

      /* Reset values */
      crtc = NULL; crtc_id = fb_id = 0;
    }

    drmModeFreeConnector(conn); conn = NULL;
  }

free_plane_res:
  drmModeFreePlaneResources(pres);
free_drm_res:
  drmModeFreeResources(dmr);  
exit_func:
  return ret;
}