//============================================================================
// Name        : main.cpp
// Author      : Akram Ansari
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <limits.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <fastmp4parser/parser.hpp>

using std::string;

void parse(string filename, Box *mp4box) {
	printf("\n\n----------MP4 Parse (%s)------------\n", filename.c_str());
	std::ifstream f(filename, f.in | f.binary);
	if (!f) {
		std::cerr << "Cannot open file.\n";
	} else {
		mp4box->parse(f, INT_MAX, 0);
		f.close();
	}
}

void mp4_stats(unsigned long long total_file_size, Box *mp4box) {

	printf("\n\n----------MP4 Info------------\n");

	/*
	MovieHeaderBox *mvhd_box =
			dynamic_cast<MovieHeaderBox*>(mp4box->get("moov")->get("mvhd"));
	*/
	Box* moov = mp4box->get("moov");
	if (!moov) {
		fprintf(stderr, "Error: 'moov' box not found\n");
		return;
	}
	Box* mvhd = moov->get("mvhd");
	if (!mvhd) {
     	   fprintf(stderr, "Error: 'mvhd' box not found\n");
	        return;
	}
	MovieHeaderBox *mvhd_box = dynamic_cast<MovieHeaderBox*>(mvhd);
	// end aaron changes

	double d = double(mvhd_box->duration) / mvhd_box->timescale;
	int hours = d / 3600;
	int minutes = (d - hours * 3600) / 60;
	double seconds = d - hours * 3600 - minutes * 60;
	printf("Duration: %.2d:%.2d:%2.3f\n", hours, minutes, seconds);
	printf("creation_time: %s", asctime(localtime(&mvhd_box->creation_time)));
	printf("modification_time: %s",
			asctime(localtime(&mvhd_box->modification_time)));
	if (d > 0) {
		printf("Bitrate: %.2f kb/s\n", (total_file_size / d) * 8.0 / 1000);
	}
	printf("Total Filesize: %llu bytes\n", total_file_size);

	std::vector<Box*> track_boxes = mp4box->get("moov")->getv("trak");
	for (int t = 1; t <= track_boxes.size(); t++) {
		printf("Track #%d: \n", t);

		/*
		std::vector<Box*> stsdEntries = track_boxes[t - 1]->get("mdia")->get(
				"minf")->get("stbl")->get("stsd")->getv();
		*/
		Box* mdia = track_boxes[t - 1]->get("mdia");
		std::vector<Box*> stsdEntries;
		if (mdia) {
			Box* minf = mdia->get("minf");
		        if (minf) {
		            Box* stbl = minf->get("stbl");
            		    if (stbl) {
		                Box* stsd = stbl->get("stsd");
                		if (stsd) {
		                    stsdEntries = stsd->getv();
                		}
		            }
        		}
		}


		for (Box *entry : stsdEntries) {
			printf("\tFormat: %.4s", entry->type);
			string type_str(entry->type, entry->type + 4);
			string format = track_boxes[t - 1]->get("mdia")->get("hdlr")->getp(
					"handler_subtype");
			if (format == "vide") {
				VideoSampleEntry *vsd = dynamic_cast<VideoSampleEntry*>(entry);
				printf(" (video)\n\tFrame Dimensions: %dx%d", vsd->width, vsd->height);
			} else if (format == "soun") {
				AudioSampleEntry *asd = dynamic_cast<AudioSampleEntry*>(entry);
				printf(" (audio)\n\tSample Rate: %u", asd->sample_rate);
			}
			printf("\n");
		}
		/*
		TimeToSampleBox *stts_box = dynamic_cast<TimeToSampleBox*>(track_boxes[t
				- 1]->get("mdia")->get("minf")->get("stbl")->get("stts"));
		*/
		TimeToSampleBox* stts_box = nullptr;
		mdia = track_boxes[t - 1]->get("mdia");
		if (mdia) {
			Box* minf = mdia->get("minf");
			if (minf) {
				Box* stbl = minf->get("stbl");
				if (stbl) {
					Box* stts = stbl->get("stts");
					if (stts) {
						stts_box = dynamic_cast<TimeToSampleBox*>(stts);
					}
				}
			}
		}
		// end aaron changes

		if (d > 0) {
			int sample_count = 0;
			if (stts_box) {
			    sample_count = stts_box->total_sample_count;
			}
			if (sample_count != 0) {
				printf("\tFrame Rate: %d\n", (int) (stts_box->total_sample_count / d));
			}
		}

		/*
		SampleSizeBox *stsz_box = dynamic_cast<SampleSizeBox*>(track_boxes[t
				- 1]->get("mdia")->get("minf")->get("stbl")->get("stsz"));
		*/
		SampleSizeBox* stsz_box = nullptr;
		mdia = track_boxes[t - 1]->get("mdia");
		if (mdia) {
			Box* minf = mdia->get("minf");
			if (minf) {
				Box* stbl = minf->get("stbl");
				if (stbl) {
					Box* stsz = stbl->get("stsz");
					if (stsz) {
						stsz_box = dynamic_cast<SampleSizeBox*>(stsz);
					}
				}
			}
		}

		if (d > 0) {
			if (stsz_box) {
				printf("\tBitrate: %d kb/s\n", (int)(stsz_box->total_samples_size *8.0/ (d*1000)));
			}
		}

	}
	printf("Total mdat boxes: %lu\n", mp4box->getv("mdat").size());
}

int main(int argc, char **argv) {

	// Print usage instructions if no arguments are passed
	if (argc < 2) {
		printf("Usage: fastMp4 [list_of_files]\n");
		return 0;
	}

	MainBox *mp4box = new MainBox();
	size_t total_file_size = 0;

	for (int i = 1; i < argc; i++) {
		std::ifstream f(argv[i], std::ios::in | std::ios::binary);
		if (!f.is_open()) {
		    fprintf(stderr, "Error: Cannot open file '%s'\n", argv[i]);
		    return 0;
		}

		parse(argv[i], mp4box);

		//std::ifstream f(argv[i], f.in | f.binary);
		f.seekg(0, std::ios::end);
		total_file_size += f.tellg();
		f.close();
	}

	mp4_stats(total_file_size, mp4box);
	return 0;
}
