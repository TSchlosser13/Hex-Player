/******************************************************************************
 * Hex-Player: Playback of hexagonal Images and Videos
 ******************************************************************************
 * v1.0 - 07.11.2017
 *
 * Copyright (c) 2017 Tobias Schlosser
 *  (tobias.schlosser@informatik.tu-chemnitz.de)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/


#pragma warning( disable : 4996 ) // MSVC (TODO: deprecated?)


#include <windows.h> // MSVC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// #include <GL/freeglut.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <SOIL/SOIL.h>

#include <math.h>
#include <float.h>

// #include "CHIP/Misc/Defines.h"
// #include "CHIP/Misc/Types.h"
#include "../../CHIP/Misc/Defines.h"
#include "../../CHIP/Misc/Types.h"

// TODO: *.c -> *.h
// #include "CHIP/CHIP/Hexint.h"
// #include "CHIP/CHIP/Hexarray.h"
#include "../../CHIP/CHIP/Hexint.c"
#include "../../CHIP/CHIP/Hexarray.c"

#include <libavformat/avformat.h>


bool mode_base = 1;
bool mode_HP;

bool play = false;

char me_title[64];

float rotation[3]       = { 0.0f, 0.0f, 0.0f };
float scale             =  1.0f;
float translation[2]    = { 0.0f, 0.0f };
float translation_scale = -2.5f;

float*   spatials;
iPoint2d spatials_min;
iPoint2d spatials_max;

unsigned int FPS_max       = 30;
unsigned int FPS_max_orig  = 30;
unsigned int frame_current =  0;
unsigned int frames_cnt    =  0;
unsigned int PTS_current   =  0;

unsigned int mode_polygon = 2;

unsigned int stream_index = 0;
unsigned int stream_order = 7;

AVFormatContext* format_context = NULL;
AVCodecContext*  codec_context;
AVFrame*         frame;
AVPacket         packet;

RGB_Hexarray* frames = NULL;
GLuint*       HDLs;


void calc_base(float** base, unsigned int order) {

	// TODO?
	if(mode_base) {
		base[0][0] =  1.0f;
		base[0][1] = -0.5f;
		base[1][0] =  0.0f;
		base[1][1] = M_SQRT3 / 2;
	} else {
		if(order == 1) {
			base[0][0] =     1.0f /       7;
			base[0][1] =    -2.0f /       7;
			base[1][0] =     5.0f /      (7 * M_SQRT3);
			base[1][1] =     4.0f /      (7 * M_SQRT3);
		} else if(order == 2) {
			base[0][0] =    -3.0f /      49;
			base[0][1] =    -8.0f /      49;
			base[1][0] =    13.0f /     (49 * M_SQRT3);
			base[1][1] =     2.0f /     (49 * M_SQRT3);
		} else if(order == 3) {
			base[0][0] =   -19.0f /     343;
			base[0][1] =   -18.0f /     343;
			base[1][0] =    17.0f /    (343 * M_SQRT3);
			base[1][1] =   -20.0f /    (343 * M_SQRT3);
		} else if(order == 4) {
			base[0][0] =   -55.0f /    2401;
			base[0][1] =   -16.0f /    2401;
			base[1][0] =   -23.0f /   (2401 * M_SQRT3);
			base[1][1] =   -94.0f /   (2401 * M_SQRT3);
		} else if(order == 5) {
			base[0][0] =   -87.0f /   16807;
			base[0][1] =    62.0f /   16807;
			base[1][0] =  -211.0f /  (16807 * M_SQRT3);
			base[1][1] =  -236.0f /  (16807 * M_SQRT3);
		} else if(order == 6) {
			base[0][0] =    37.0f /  117649;
			base[0][1] =   360.0f /  117649;
			base[1][0] =  -683.0f / (117649 * M_SQRT3);
			base[1][1] =  -286.0f / (117649 * M_SQRT3);
		} else if(order == 7) {
			base[0][0] =   757.0f /  823543;
			base[0][1] =  1006.0f /  823543;
			base[1][0] = -1225.0f / (823543 * M_SQRT3);
			base[1][1] =   508.0f / (823543 * M_SQRT3);
		} else {
			base[0][0] =     1.0f;
			base[0][1] =     0.0f;
			base[1][0] =     0.0f;
			base[1][1] =     1.0f;
		}
	}

}

void disp_hex(float x_base, float y_base, float radius, float offset) {
	glBegin(GL_POLYGON);
		for(unsigned int i = 0; i < 7; i++) {
			const float x = radius * cosf(i * M_PI / 3 + M_PI / 2 + offset);
			const float y = radius * sinf(i * M_PI / 3 + M_PI / 2 + offset);

			glVertex3f(x_base + x, y_base + y, 0.0f);
		}
	glEnd();
}

void disp_frame(RGB_Hexarray frame) {

	// TODO?

	float** base = malloc(2 * sizeof(float*));

	base[0] = (float*)malloc(2 * sizeof(float));
	base[1] = (float*)malloc(2 * sizeof(float));


	const unsigned int order = (unsigned int)(logf(frame.size / 3.0f) / logf(7));

	calc_base(base, order);


	float m = -FLT_MAX;
	float radius;
	float offset;

	if(mode_base) {
		radius = 1.0f / M_SQRT3;
		offset = 0.0f;
	} else {
		radius = sqrtf(base[0][1] * base[0][1] + base[1][1] * base[1][1]) / M_SQRT3;
		offset = atan2f(base[1][0] - base[1][1], base[0][0] - base[0][1]);
	}

	for(unsigned int i = 0; i < frame.size; i++) {
		fPoint2d hex;
		fPoint2d psf;


		if(mode_base) {
			psf = getSpatial(   Hexint_init(i, 0) );
		} else {
			psf = getFrequency( Hexint_init(i, 0) );
		}

		hex.x = psf.x * base[0][0] + psf.y * base[0][1];
		hex.y = psf.x * base[1][0] + psf.y * base[1][1];


		if(hex.x > m)
			m = hex.x;

		if(hex.y > m)
			m = hex.y;


		glColor3f(frame.p[i][0] / 255.0f, frame.p[i][1] / 255.0f, frame.p[i][2] / 255.0f);

		disp_hex(hex.x, hex.y, radius, offset);
	}

	scale = 1 / (m * (1 + 0.8f / order));


	free(base[0]);
	free(base[1]);

	free(base);

}

void Hexdisp_init() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_SMOOTH);


	putchar('\n');

	if(!mode_HP) {
		HDLs = (GLuint*)malloc(frames_cnt * sizeof(GLuint));

		for(unsigned int i = 0; i < frames_cnt; i++) {
			printf("[Hex-Player] disp_frame(f%u)\n", i);

			HDLs[i] = glGenLists(1);

			glNewList(HDLs[i], GL_COMPILE);
				disp_frame(frames[i]);
			glEndList();
		}
	}
}


// FPS

unsigned int FPS;
unsigned int f         = 0;
unsigned int time_base = 0;
unsigned int update    = 0;

void calc_FPS() {
	const unsigned int time = glutGet(GLUT_ELAPSED_TIME);

	if(time_base) {
		f++;
	} else {
		time_base = time;
		update    = time;

		return;
	}


	if(play && time - update > 1000 / FPS_max) {
		if(!mode_HP) {
			if(frame_current < frames_cnt - 1) {
				frame_current++;
			} else {
				frame_current = 0;
			}
		} else {
			if(frame_current <= frames_cnt)
				frame_current++;
		}


		sprintf(me_title, "Hex-Player v1.0: FPS = %u/%u, " \
			"frame_current = %u", FPS, FPS_max, frame_current);

		glutSetWindowTitle(me_title);


		update = time;
	}


	const unsigned int time_diff = time - time_base;

	if(time_diff > 999) {
		FPS = (f * 1000) / time_diff;

		if(!play) {
			sprintf(me_title, "Hex-Player v1.0: FPS = %u/%u, " \
				"frame_current = %u", FPS, FPS_max, frame_current);

			glutSetWindowTitle(me_title);
		}

		f         = 0;
		time_base = time;
	}
}


void read_next_frame() {
	int ret = av_read_frame(format_context, &packet);

	if(packet.stream_index != stream_index) {
		av_packet_unref(&packet);

		read_next_frame();
		return;
	}

	if(ret == AVERROR_EOF) {
		av_seek_frame(format_context, stream_index, 0, AVSEEK_FLAG_BACKWARD);

		ret = av_read_frame(format_context, &packet);

		if(packet.stream_index != stream_index) {
			av_packet_unref(&packet);

			read_next_frame();
			return;
		}
	}


	int got_frame = 0;

	ret = avcodec_decode_video2(codec_context, frame, &got_frame, &packet); // deprecated (TODO?)

	av_packet_unref(&packet);

	if(ret < 0) {
		fputs("\n[Hex-Player] Error decoding frame\n", stderr);
		return;
	}


	if(got_frame) {
		printf(
			"[Hex-Player] frame->coded_picture_number = %8i, frame->pts = %8li:"
			" frame->width x frame->height = %i x %i (read_next_frame: avcodec_decode_video2)\n",
			 frame->coded_picture_number, frame->pts, frame->width, frame->height);


		// Clear frames[0]
		for(unsigned int p = 0; p < frames[0].size; p++) {
			frames[0].p[p][0] =  16;
			frames[0].p[p][1] = 128;
			frames[0].p[p][2] = 128;
		}


		// frame->data -> frames[0]

		const int width_base  = (int)roundf((frame->width  - (spatials_max.x - spatials_min.x)) / 2);
		const int height_base = (int)roundf((frame->height - (spatials_max.y - spatials_min.y)) / 2);

		// 4:4:4
		if(frame->linesize[0] == frame->linesize[1]) {
			for(unsigned int i = 0; i < frames[0].size; i++) {
				const int w = width_base  + (int)spatials[2 * i];
				const int h = height_base + (int)spatials[2 * i + 1];

				if(w >= 0 && h >= 0 && w < frame->width && h < frame->height) {
					const unsigned int p = h * frame->linesize[0] + w;

					frames[0].p[i][0] = frame->data[0][p];
					frames[0].p[i][1] = frame->data[1][p];
					frames[0].p[i][2] = frame->data[2][p];
				}
			}
		// 4:2:0
		} else {
			for(unsigned int i = 0; i < frames[0].size; i++) {
				const int w = width_base  + (int)spatials[2 * i];
				const int h = height_base + (int)spatials[2 * i + 1];

				if(w >= 0 && h >= 0 && w < frame->width && h < frame->height) {
					const unsigned int p      =  h      * frame->linesize[0] +  w;
					const unsigned int p_CbCr = (h / 2) * frame->linesize[1] + (w / 2);

					frames[0].p[i][0] = frame->data[0][p];
					frames[0].p[i][1] = frame->data[1][p_CbCr];
					frames[0].p[i][2] = frame->data[2][p_CbCr];
				}
			}
		}


		// YCbCr -> RGB
		for(unsigned int p = 0; p < frames[0].size; p++) {
			const float C = 1.164383f * (frames[0].p[p][0] -  16);
			const float D =              frames[0].p[p][1] - 128;
			const float E =              frames[0].p[p][2] - 128;

			const int R = (int)roundf( C                 + 1.596027f * E );
			const int G = (int)roundf( C - 0.391762f * D - 0.812968f * E );
			const int B = (int)roundf( C + 2.017232f * D                 );

			if(R < 0) {
				frames[0].p[p][0] =   0;
			} else if(R > 255) {
				frames[0].p[p][0] = 255;
			} else {
				frames[0].p[p][0] = R;
			}
			if(G < 0) {
				frames[0].p[p][1] =   0;
			} else if(G > 255) {
				frames[0].p[p][1] = 255;
			} else {
				frames[0].p[p][1] = G;
			}
			if(B < 0) {
				frames[0].p[p][2] =   0;
			} else if(B > 255) {
				frames[0].p[p][2] = 255;
			} else {
				frames[0].p[p][2] = B;
			}
		}


		frames_cnt++;

		PTS_current = frame->pts;


		av_frame_unref(frame); // refcounted_frames
	}
}

void displayFunc() {
	calc_FPS();


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glTranslatef(translation[0], translation[1], translation_scale);
	glScalef(scale, scale, scale);
	glRotatef(rotation[0], 1.0f, 0.0f, 0.0f);
	glRotatef(rotation[1], 0.0f, 1.0f, 0.0f);
	glRotatef(rotation[2], 0.0f, 0.0f, 1.0f);

	if(!mode_HP) {
		glCallList(HDLs[frame_current]);
	} else {
		if(!frames_cnt || frame_current >= frames_cnt)
			read_next_frame();

		disp_frame(frames[0]); // keep only one frame (TODO?)
	}

	glFlush();
	glutSwapBuffers();
}


void keyboardFunc(unsigned char key, int x, int y) {
	// Reset
	if(key == 'r' || key == 'R') {
		rotation[0]    = rotation[1]    = rotation[2] =  0.0f;
		translation[0] = translation[1]               =  0.0f;
		translation_scale                             = -2.5f;

		FPS_max = FPS_max_orig;

		if(!mode_HP) {
			frame_current = 0;
		} else {
			av_seek_frame(format_context, stream_index, 0, AVSEEK_FLAG_BACKWARD);
		}

		mode_polygon = 2;
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Exit (Esc)
	if(key == 27) exit(EXIT_SUCCESS);


	// Rotation
	if(key == 'x') rotation[0] -= 5.0f;
	if(key == 'X') rotation[0] += 5.0f;
	if(key == 'y') rotation[1] -= 5.0f;
	if(key == 'Y') rotation[1] += 5.0f;
	if(key == 'z') rotation[2] -= 5.0f;
	if(key == 'Z') rotation[2] += 5.0f;

	// Scale
	if(key == 'g') translation_scale += 0.1f * translation_scale / -2.5f;
	if(key == 'G') translation_scale -= 0.1f * translation_scale / -2.5f;

	// Translation
	if(key == 'w' || key == 'W') translation[1] -= 0.1f * translation_scale / -2.5f;
	if(key == 's' || key == 'S') translation[1] += 0.1f * translation_scale / -2.5f;
	if(key == 'a' || key == 'A') translation[0] += 0.1f * translation_scale / -2.5f;
	if(key == 'd' || key == 'D') translation[0] -= 0.1f * translation_scale / -2.5f;


	// Polygon Mode
	if(key == 'p' || key == 'P') {
		if(!mode_polygon) {
			mode_polygon++;

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		} else if(mode_polygon == 1) {
			mode_polygon++;

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		} else {
			mode_polygon = 0;

			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		}
	}

	// Screenshot
	if(key == 'b' || key == 'B') {
		       time_t time_raw  = time(NULL);
		struct tm*    time_info = localtime(&time_raw);


		char title[32];

		strftime(title, 32, "%Y-%m-%d_%H-%M-%S.bmp", time_info);


		const unsigned int me_width  = glutGet(GLUT_WINDOW_WIDTH);
		const unsigned int me_height = glutGet(GLUT_WINDOW_HEIGHT);

		SOIL_save_screenshot(title, SOIL_SAVE_TYPE_BMP, 0, 0, me_width, me_height);
	}




	// FPS

	if(key == 'j' || key == 'J') FPS_max++;

	if(key == 'k' || key == 'K') {
		if(FPS_max > 1)
			FPS_max--;
	}


	// Playback

	if(key == 'c' || key == 'C') play = true;
	if(key == 'v' || key == 'V') play = false;


	// Frame Selection

	if(key == 'n' || key == 'N') {
		if(!mode_HP) {
			if(frame_current > 0) {
				frame_current--;
			} else {
				frame_current = frames_cnt - 1;
			}
		} else {
			frame_current++;

			av_seek_frame(format_context, stream_index, PTS_current - 1, AVSEEK_FLAG_BACKWARD);
		}


		sprintf(me_title, "Hex-Player v1.0: FPS = %u/%u, " \
			"frame_current = %u", FPS, FPS_max, frame_current);

		glutSetWindowTitle(me_title);
	}

	if(key == 'm' || key == 'M') {
		if(!mode_HP) {
			if(frame_current < frames_cnt - 1) {
				frame_current++;
			} else {
				frame_current = 0;
			}
		} else {
			frame_current++;

			av_seek_frame(format_context, stream_index, PTS_current + 1, AVSEEK_FLAG_FRAME);
		}


		sprintf(me_title, "Hex-Player v1.0: FPS = %u/%u, " \
			"frame_current = %u", FPS, FPS_max, frame_current);

		glutSetWindowTitle(me_title);
	}




	rotation[0] = fmodf(rotation[0], 360);
	rotation[1] = fmodf(rotation[1], 360);
	rotation[2] = fmodf(rotation[2], 360);
}

void reshapeFunc(int w, int h) {
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(40.0, w / h, 0.001, 64.0);
	glMatrixMode(GL_MODELVIEW);
}


// Menu

void menu(int option) {
	switch(option) {
		case  0 : keyboardFunc( 'p', 0, 0 ); break; // Polygon Mode
		case  1 : keyboardFunc( 'b', 0, 0 ); break; // Screenshot
		case  2 : keyboardFunc( 'c', 0, 0 ); break; // Start Playback
		case  3 : keyboardFunc( 'v', 0, 0 ); break; // Stop  Playback
		case  4 : keyboardFunc( 'r', 0, 0 ); break; // Reset
		case  5 : keyboardFunc(  27, 0, 0 ); break; // Exit (Esc)
		default :                            break;
	}
}

void create_menu() {
	/*int ID = */glutCreateMenu(menu);

	glutAddMenuEntry("Polygon Mode (p, P)",   0);
	glutAddMenuEntry("Screenshot (b, B)",     1);
	glutAddMenuEntry("Start Playback (c, C)", 2);
	glutAddMenuEntry("Stop Playback (v, V)",  3);
	glutAddMenuEntry("Reset (r, R)",          4);
	glutAddMenuEntry("Exit (Esc)",            5);

	glutAttachMenu(GLUT_RIGHT_BUTTON);
}


void Hexdisp(int argc, char** argv, unsigned int x, unsigned int y,
 unsigned int width, unsigned int height) {
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(x, y);
	glutInitWindowSize(width, height);

	glutCreateWindow("Hex-Player v1.0");

	Hexdisp_init();


	glutDisplayFunc(displayFunc);
	glutIdleFunc(displayFunc);
	glutKeyboardFunc(keyboardFunc);
	glutReshapeFunc(reshapeFunc);

	create_menu();


	glutMainLoop();
}


int main(int argc, char** argv) {
	int ret;


	if(argc < 2) {
		printf(
			"[Hex-Player] Usage: %s <hex. Image|Reference|Video>"
			" [ <stream_index=(>=0)> <stream_order=(1-7)> ]\n",
			 argv[0]);

		return 1;
	}

	if(argc > 2) {
		stream_index = (unsigned int)atoi(argv[2]);
		stream_order = (unsigned int)atoi(argv[3]);
	}


	char* dot = strrchr(argv[1], '.');

	// Image / Reference
	if(dot && !strcmp(dot, ".dat")) {
		bool         finished_scanning = false;
		FILE*        files;
		char         files_path[64];
		unsigned int frames_size       = 0;


		mode_HP = 0;


		files = fopen(argv[1], "r");

		for(int i = strlen(argv[1]) - 1; i > -1; i--) {
			if(argv[1][i] == '/') {
				memcpy(files_path, argv[1], i);
				files_path[i] = '\0';

				break;
			} else if(!i) {
				files_path[0] = '\0';
			}
		}


		while(!feof(files)) {
			FILE*         file;
			char          file_name[64];
			char          file_path_name[64]; // reference only
			char          junk[16];
			unsigned int* data      = NULL;
			unsigned int  data_size = 0;


			ret = fscanf(files, "%s", file_name);

			if(ret <= 0 || file_name[0] == '#')
				continue;

			// Image / Reference
			if(file_name[0] == '<') {
				printf("[Hex-Player] Loading \"%s\": ", argv[1]);
				file = fopen(argv[1], "r");
				ret  = fscanf(file, "%s", junk); // junk: "<("

				finished_scanning = true;
			} else {
				sprintf(file_path_name, "%s/%s", files_path, file_name);
				printf("[Hex-Player] Loading \"%s\": ", file_path_name);
				file = fopen(file_path_name, "r");
				ret  = fscanf(file, "%s", junk); // junk: "<("
			}


			while(!feof(file)) {
				data_size += 3;
				data       = (unsigned int*)realloc(data, data_size * sizeof(unsigned int));

				ret = fscanf(file, "%u %u %u", &data[data_size - 3], &data[data_size - 2], &data[data_size - 1]);

				ret = fscanf(file, "%s", junk); // junk: "),(" or ")>"
			}

			fclose(file);


			const unsigned int order = (unsigned int)(logf(data_size / 3.0f) / logf(7));

			frames_size++;
			frames = (RGB_Hexarray*)realloc(frames, frames_size * sizeof(RGB_Hexarray));

			Hexarray_init(&frames[frames_size - 1], order, 0);

			for(unsigned int p = 0; p < frames[frames_size - 1].size; p++) {
				const unsigned int p3 = p * 3;

				frames[frames_size - 1].p[p][0] = data[p3];
				frames[frames_size - 1].p[p][1] = data[p3 + 1];
				frames[frames_size - 1].p[p][2] = data[p3 + 2];
			}

			frames_cnt++;


			printf("order = %u, frames_cnt = %u\n", order, frames_cnt);


			if(finished_scanning)
				break;
		}


		fclose(files);
	// Video
	} else {
		mode_HP = 1;


		av_register_all();

		if(avformat_open_input(&format_context, argv[1], 0, 0) < 0) {
			fputs("\n[Hex-Player] Could not open input file\n", stderr);
			return 1;
		}

		if(avformat_find_stream_info(format_context, 0) < 0) {
			fputs("\n[Hex-Player] Failed to retrieve input stream information\n", stderr);
			return 1;
		}

		av_dump_format(format_context, 0, argv[1], 0);


		AVStream*          stream           = format_context->streams[stream_index];
		AVCodecParameters* codec_parameters = stream->codecpar;

		// TODO: AVMEDIA_TYPE_AUDIO?
		if(codec_parameters->codec_type != AVMEDIA_TYPE_VIDEO) {
			return 0;
		}

		FPS_max_orig = (unsigned int)roundf((float)stream->avg_frame_rate.num / stream->avg_frame_rate.den);
		FPS_max      = FPS_max_orig;




		AVDictionary* options = NULL;

		av_dict_set(&options, "refcounted_frames", "1", 0);


		AVCodec* codec = avcodec_find_decoder(codec_parameters->codec_id);
		if(!codec) {
			fputs("\n[Hex-Player] Failed to find codec\n", stderr);
			return 1;
		}

		codec_context = avcodec_alloc_context3(codec);
		if(!codec_context) {
			fputs("\n[Hex-Player] Failed to allocate codec context\n", stderr);
			return 1;
		}

		ret = avcodec_parameters_to_context(codec_context, codec_parameters);
		if(ret < 0) {
			fputs("\n[Hex-Player] Failed to copy codec parameters to codec context\n", stderr);
			return 1;
		}

		ret = avcodec_open2(codec_context, codec, &options);
		if(ret < 0) {
			fputs("\n[Hex-Player] Failed to open codec context\n", stderr);
			return 1;
		}


		frames = (RGB_Hexarray*)malloc(sizeof(RGB_Hexarray)); // keep only one frame (TODO?)

		Hexarray_init(&frames[0], stream_order, 0);


		frame = av_frame_alloc();
		if(!frame) {
			fputs("\n[Hex-Player] Could not allocate frame\n", stderr);
			return 1;
		}


		av_init_packet(&packet);

		// av_init_packet
		packet.data = NULL;
		packet.size = 0;




		const unsigned int size = pow(7, stream_order);

		Hexint   hi;
		fPoint2d ps;


		spatials = (float*)malloc(2 * size * sizeof(float));

		for(unsigned int i = 0; i < size; i++) {
			hi = Hexint_init(i, 0);

			ps = getSpatial(hi);

			if(ps.x < spatials_min.x) {
				spatials_min.x = (int)ps.x;
			} else if(ps.x > spatials_max.x) {
				spatials_max.x = (int)ps.x;
			}
			if(ps.y < spatials_min.y) {
				spatials_min.y = (int)ps.y;
			} else if(ps.y > spatials_max.y) {
				spatials_max.y = (int)ps.y;
			}
		}

		for(unsigned int i = 0; i < size; i++) {
			ps = getSpatial(Hexint_init(i, 0));

			if(ps.y > 1.0f) {
				ps.x -= roundf((ps.y - 1) / 2);
			} else if(ps.y < 0.0f) {
				ps.x -= roundf(ps.y / 2);
			}

			ps.x -= spatials_min.x;
			ps.y  = spatials_max.y - ps.y;

			spatials[2 * i]     = (unsigned int)ps.x;
			spatials[2 * i + 1] = (unsigned int)ps.y;
		}
	}


	Hexdisp(0, NULL, 64, 64, 720, 720);


	return 0;
}

