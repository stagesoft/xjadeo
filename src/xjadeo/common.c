/* xjadeo - common access functions
 *
 * Copyright (C) 2014 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "xjadeo.h"
#include <stdio.h>
#include <stdarg.h>

/* Video file information - set by C++ VideoFileInput */
int               movie_width  = 640;
int               movie_height = 360;
float             movie_aspect = 640.0 / 360.0;
double            framerate = 1.0;
int64_t           frames = 1;
int               have_dropframes = 0;

/* Configuration flags - set by C++ ConfigurationManager */
int               want_quiet = 0;
int               want_verbose = 0;
int               want_debug = 0;
int               want_dropframes = 0;
int               want_autodrop = 0;

/* MIDI clock conversion (for SMPTEWrapper compatibility) */
int               midi_clkconvert = 0;

/* Legacy UI state variables */
int64_t           userFrame = 0;
int               interaction_override = 0; // disable some options
int               force_redraw = 0;
int               OSD_mode = 0; // change via keystroke (OSD_BOX default)
int               OSD_fx = 0, OSD_fy = 0;
int               OSD_sx = 0, OSD_sy = 0;
int               OSD_tx = 0, OSD_ty = 0;

/* Display loop variables (from main.c/xjadeo.c) */
int               loop_flag = 1; // main event loop flag
int               loop_run = 1;   // video update enable

/* Display options */
int               hide_mouse = 0;
int               want_letterbox = 0;

/* Video buffer (from xjadeo.c) */
uint8_t*          buffer = NULL;

/* Video dimensions (from xjadeo.c) */
int               ffctv_width = 640;
int               ffctv_height = 360;

/* Display function stubs (from display.c/xjadeo.c) */
void xapi_open(void* d) {
    // Stub for file open - C++ codebase uses VideoFileInput instead
    // Note: C display backends use void* parameter
    (void)d;
}

void xapi_close(void* d) {
    // Stub for file close - C++ codebase uses VideoFileInput instead
    (void)d;
}

/* JACK transport stubs (JACK support removed) */
void jackt_rewind(void) {
    // JACK support removed
}

void jackt_toggle(void) {
    // JACK support removed
}

/* Display window stubs */
void start_ontop(void) {
    // Stub - window management handled by C++ OpenGLDisplay
}

void start_fullscreen(void) {
    // Stub - window management handled by C++ OpenGLDisplay
}

/* Additional C globals (from xjadeo.c/main.c/configfile.c) */
char*            OSD_fontfile = NULL;
int              want_nosplash = 0;
int              splashed = 0;
int              index_progress = 0;
double           ts_offset = 0.0;
double           smpte_offset = 0.0;

/* Function implementation for have_open_file (declared in xjadeo.h) */
int have_open_file(void) {
    // Stub - file open status now handled by C++ VideoFileInput
    return 0;
}

/* OSD function stubs (from display.c - OSD now handled by C++ OSDManager) */
void OSD_frame(int64_t frame) { (void)frame; }
void OSD_smpte(int h, int m, int s, int f) { (void)h; (void)m; (void)s; (void)f; }
void OSD_text(const char* text) { (void)text; }
void OSD_msg(const char* msg) { (void)msg; }
void OSD_nfo_tme(int x, int y, int64_t frame, double fps) { (void)x; (void)y; (void)frame; (void)fps; }
void OSD_nfo_geo(int x, int y, int w, int h, float aspect) { (void)x; (void)y; (void)w; (void)h; (void)aspect; }
void dispFrame(int64_t frame) { (void)frame; }

/* JACK transport stubs (additional) */
void jackt_start(void) {
    // JACK support removed
}

void jackt_stop(void) {
    // JACK support removed
}

// Stub for remote_printf (remote.c removed, remote control migrated to C++)
// This prevents linker errors for functions in common.c that reference it
// JACK/LTC support has been removed, so these functions are no longer functional
static void remote_printf_stub(int val, const char *format, ...) {
	// Remote control now handled by C++ OSCRemoteControl
	// This stub prevents linker errors but does nothing
	(void)val;
	(void)format;
	// Could optionally log to stderr if needed for debugging
}

// Provide remote_printf if not defined elsewhere (remote.c removed)
#ifndef HAVE_REMOTE_PRINTF
void remote_printf(int val, const char *format, ...) {
	remote_printf_stub(val, format);
}
#endif

// Stub for remote_notify (remote.c removed)
void remote_notify(int mode, int rv, const char *format, ...) {
	// Remote control now handled by C++ OSCRemoteControl
	// This stub prevents linker errors but does nothing
	(void)mode;
	(void)rv;
	(void)format;
}

void INT_sync_to_jack (int remote_msg) {
	// JACK support removed - this function is no longer functional
	// Kept for compatibility but does nothing
	(void)remote_msg;
#ifdef HAVE_MIDI
	if (midi_connected()) midi_close();
#endif
#ifdef HAVE_LTC
	if (ltcjack_connected()) close_ltcjack();
#endif
	// open_jack(); // JACK removed
	if (remote_msg) {
		remote_printf (405,"JACK support has been removed");
	}
}

void INT_sync_to_ltc (char *port, int remote_msg) {
	// JACK/LTC support removed - this function is no longer functional
	// Kept for compatibility but does nothing
	(void)port;
	(void)remote_msg;
	// if (jack_connected()) close_jack(); // JACK removed
#ifdef HAVE_MIDI
	if (midi_connected()) midi_close();
#endif
#ifdef HAVE_LTC
	// open_ltcjack(port); // LTC-JACK removed
	if (remote_msg) {
		remote_printf (499,"LTC-jack support has been removed");
	}
#else
	if (remote_msg) {
		remote_printf (499,"LTC-jack is not available.");
	}
#endif
}

// Stub JACK functions (JACK support removed)
// These prevent linker errors for common.c functions that reference JACK
static int jack_connected_stub(void) { return 0; }
static void open_jack_stub(void) { }
static void close_jack_stub(void) { }

// Provide JACK stubs if not defined elsewhere (jack.c removed)
#ifndef HAVE_JACK_FUNCTIONS
int jack_connected(void) { return jack_connected_stub(); }
void open_jack(void) { open_jack_stub(); }
void close_jack(void) { close_jack_stub(); }
#endif

// Stub LTC-JACK functions (ltc-jack.c removed, JACK support removed)
// These prevent linker errors for common.c functions that reference LTC-JACK
static int ltcjack_connected_stub(void) { return 0; }
static void open_ltcjack_stub(char *autoconnect) { (void)autoconnect; }
static void close_ltcjack_stub(void) { }

// Provide LTC-JACK stubs if not defined elsewhere (ltc-jack.c removed)
#ifndef HAVE_LTCJACK_FUNCTIONS
int ltcjack_connected(void) { return ltcjack_connected_stub(); }
void open_ltcjack(char *autoconnect) { open_ltcjack_stub(autoconnect); }
void close_ltcjack(void) { close_ltcjack_stub(); }
#endif

// Stub MIDI functions (midi.c removed, C++ version complete)
// These prevent linker errors for common.c functions that reference MIDI
// NOTE: C++ code uses MIDISyncSource class, not these C functions
static int midi_connected_stub(void) { return 0; }
static int64_t midi_poll_frame_stub(void) { return -1; }
static void midi_open_stub(char *midiid) { (void)midiid; }
static void midi_close_stub(void) { }
static const char* midi_driver_name_stub(void) { return "none"; }

// Provide MIDI stubs if not defined elsewhere (midi.c removed)
#ifndef HAVE_MIDI_FUNCTIONS
int midi_connected(void) { return midi_connected_stub(); }
int64_t midi_poll_frame(void) { return midi_poll_frame_stub(); }
void midi_open(char *midiid) { midi_open_stub(midiid); }
void midi_close(void) { midi_close_stub(); }
const char* midi_driver_name(void) { return midi_driver_name_stub(); }
int midi_choose_driver(const char *driver) { 
	// JACK-MIDI support removed - reject it
	if (driver && !strcmp(driver, "JACK-MIDI")) {
		return 0; // Reject JACK-MIDI
	}
	(void)driver; 
	return 0; // Stub for removed midi.c
}
#endif

void ui_sync_none () {
	if (interaction_override&OVR_MENUSYNC) return;
	if (jack_connected()) close_jack();
#ifdef HAVE_MIDI
	if (midi_connected()) midi_close();
#endif
#ifdef HAVE_LTC
	if (ltcjack_connected()) close_ltcjack();
#endif
}

void ui_sync_manual (float percent) {
	if (interaction_override&OVR_MENUSYNC) return;
	if (frames < 1) return;
	ui_sync_none();
	if (percent <= 0.f) percent = 0.f;
	if (percent >= 100.f) percent = 100.f;
	userFrame = rint((frames - 1.f) * percent / 100.f);
}

void ui_sync_to_jack () {
	if (interaction_override&OVR_MENUSYNC) return;
	INT_sync_to_jack (0);
}

void ui_sync_to_ltc () {
	if (interaction_override&OVR_MENUSYNC) return;
	INT_sync_to_ltc (NULL, 0);
}

static void ui_sync_to_mtc (const char *driver) {
	if (interaction_override&OVR_MENUSYNC) return;
	if (jack_connected()) close_jack();
#ifdef HAVE_LTC
	if (ltcjack_connected()) close_ltcjack();
#endif
#ifdef HAVE_MIDI
	if (midi_connected() && strcmp (midi_driver_name(), driver)) {
		midi_close();
	}
	if (!midi_connected()) {
		midi_choose_driver (driver);
		midi_open ("-1");
	}
#endif
}

void ui_sync_to_mtc_jack () {
	// JACK-MIDI support removed - redirect to ALSA Sequencer if available
	// or do nothing if no MIDI driver is available
#ifdef HAVE_MIDI
	ui_sync_to_mtc ("ALSA-Sequencer");
#else
	// No MIDI support - do nothing
#endif
}
void ui_sync_to_mtc_portmidi () {
	ui_sync_to_mtc ("PORTMIDI");
}
void ui_sync_to_mtc_alsaraw () {
	ui_sync_to_mtc ("ALSA-RAW-MIDI");
}
void ui_sync_to_mtc_alsaseq () {
	ui_sync_to_mtc ("ALSA-Sequencer");
}

enum SyncSource ui_syncsource() {
	// JACK support removed - no longer check for JACK transport
	// if (jack_connected()) {
	//	return SYNC_JACK;
	// }
#ifdef HAVE_MIDI
	if (ltcjack_connected()) {
		return SYNC_LTC;
	}
#endif
#ifdef HAVE_MIDI
	if (midi_connected() && !strcmp (midi_driver_name(), "PORTMIDI")) {
		return SYNC_MTC_PORTMIDI;
	}
	// JACK-MIDI support removed - this case should never occur
	// if (midi_connected() && !strcmp (midi_driver_name(), "JACK-MIDI")) {
	//	return SYNC_MTC_JACK;
	// }
	if (midi_connected() && !strcmp (midi_driver_name(), "ALSA-RAW-MIDI")) {
		return SYNC_MTC_ALSARAW;
	}
	if (midi_connected() && !strcmp (midi_driver_name(), "ALSA-Sequencer")) {
		return SYNC_MTC_ALSASEQ;
	}
#endif
	return SYNC_NONE;
}

void ui_osd_clear () {
	OSD_mode = OSD_BOX; // XXX retain message when indexing or file closed?
	force_redraw = 1;
}

void ui_osd_offset_cycle () {
	OSD_mode &= ~(OSD_NFO | OSD_GEO);
	if (OSD_mode & OSD_OFFF) {
		OSD_mode &= ~(OSD_OFFF | OSD_OFFS);
	}
	else if (OSD_mode & OSD_OFFS) {
		OSD_mode &= ~OSD_OFFS;
		OSD_mode |= OSD_OFFF;
	} else {
		OSD_mode &= ~OSD_OFFF;
		OSD_mode |= OSD_OFFS;
	}
	force_redraw = 1;
}

void ui_osd_offset_tc () {
	OSD_mode &= ~(OSD_OFFF | OSD_NFO | OSD_GEO);
	OSD_mode |= OSD_OFFS;
	force_redraw = 1;
}

void ui_osd_offset_fn () {
	OSD_mode &= ~(OSD_OFFS | OSD_NFO | OSD_GEO);
	OSD_mode |= OSD_OFFF;
	force_redraw = 1;
}

void ui_osd_offset_none () {
	OSD_mode &= ~(OSD_OFFF | OSD_OFFS);
	force_redraw = 1;
}

void ui_osd_tc () {
	OSD_mode ^= OSD_SMPTE;
	force_redraw = 1;
}

void ui_osd_fn () {
	if (OSD_mode & OSD_FRAME) {
		OSD_mode &= ~(OSD_VTC | OSD_FRAME);
	} else if (OSD_mode & OSD_VTC) {
		OSD_mode &= ~OSD_VTC;
		OSD_mode |= OSD_FRAME;
	} else {
		OSD_mode &= ~OSD_FRAME;
		OSD_mode |= OSD_VTC;
	}
	force_redraw = 1;
}

void ui_osd_vtc_fn () {
	OSD_mode &= ~OSD_VTC;
	OSD_mode |= OSD_FRAME;
	force_redraw = 1;
}

void ui_osd_vtc_tc () {
	OSD_mode &= ~OSD_FRAME;
	OSD_mode |= OSD_VTC;
	force_redraw = 1;
}

void ui_osd_vtc_off () {
	OSD_mode &= ~(OSD_VTC | OSD_FRAME);
	force_redraw = 1;
}

void ui_osd_box () {
	OSD_mode ^= OSD_BOX;
	force_redraw = 1;
}

void ui_osd_geo () {
	ui_osd_offset_none();
	OSD_mode &= ~OSD_NFO;
	OSD_mode ^= OSD_GEO;
	force_redraw = 1;
}

void ui_osd_fileinfo () {
	ui_osd_offset_none();
	OSD_mode &= ~OSD_GEO;
	OSD_mode ^= OSD_NFO;
	force_redraw = 1;
}

void ui_osd_pos () {
	OSD_mode ^= OSD_POS;
	force_redraw = 1;
}

void ui_osd_outofrange () {
	OSD_mode ^= OSD_VTCOOR;
	if (OSD_mode & OSD_VTCOOR) {
		OSD_mode |= OSD_VTC;
		OSD_mode &= ~OSD_FRAME;
	}
	force_redraw = 1;
}

void ui_osd_permute () {
	const int t1 = OSD_sy;
	OSD_sy = OSD_fy;
	OSD_fy = t1;
	force_redraw = 1;
}
