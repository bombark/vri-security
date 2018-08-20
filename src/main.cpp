#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

using namespace std;

#include "record.hpp"

/*

#include <sys/types.h>
#include <string.h>
#include <microhttpd.h>

typedef std::string String;

struct SyncCallCtx;

struct Unit {
	virtual void start()=0;
	virtual void stop()=0;
	virtual void exec(SyncCallCtx& ctx)=0;
};


struct SyncCallCtx {
	Unit* unit;
	String input;
	String output;

	SyncCallCtx(Unit* unit){
		this->unit = unit;
		this->unit->start();
	}

	~SyncCallCtx(){
		this->unit->stop();
		this->unit = NULL;
	}

	int exec(String input){
		this->input = input;
		this->unit->exec(*this);
	}
};


struct TestUnit : Unit {
	void start(){
		cout << "stated\n";
	}

	void stop(){
		cout << "stopped\n";
	}

	void exec(SyncCallCtx& ctx){
		ctx.output = "<html><body><img src='fs/figure.jpg' width=256></body></html>";
	}
};


struct FsUnit : Unit {
	void start(){}

	void stop(){}

	void exec(SyncCallCtx& ctx){
		String url = ctx.input.substr(4);
		FILE* fd = fopen(url.c_str(), "r");
		if ( fd ){
			char buf[4096];
			String& out = ctx.output;

			size_t read;
			while ( (read = fread(buf,1,4096,fd)) > 0 ){
				size_t cur = out.size();
				out.resize( cur + read );
				char* dst = (char*) out.c_str();
				memcpy(dst+cur,buf,read);
			}
			fclose(fd);
		}
	}
};






struct HttpCtrl {
	struct MHD_Daemon *daemon;

	HttpCtrl(int port){
		cerr << "[Httpd Control]: starting;\n";
		this->daemon = MHD_start_daemon (
			MHD_USE_SELECT_INTERNALLY, port, NULL, NULL,
			&callback, NULL, MHD_OPTION_END
		);
		if ( this->daemon == NULL ){
			cerr << "[Httpd Control]: error;";
			exit(1);
		}
	}

	~HttpCtrl(){this->release();}


	Unit* route(String url){
		Unit* unit = NULL;
		if ( url == "/" ){
			unit = new TestUnit();
		} else if ( url == "/camera" ){
			unit = new TestUnit();
		} else {
			String first = url.substr(0,4);
			if ( first == "/fs/" )
				unit = new FsUnit();
		}
		return unit;
	}



	static int callback (
		void *_self, struct MHD_Connection *conn,
		const char *url, const char *method,
		const char *version, const char *upload_data,
		size_t *upload_data_size, void **con_cls
	){
		HttpCtrl& self = *( (HttpCtrl*)_self );



		struct MHD_Response *response;

		int ret;
		Unit* unit = self.route(url);
		if ( unit == NULL ){
			printf("[%s]: not found\n",url);
			const char *page = "<html><body>Hello, browser!</body></html>";
			response = MHD_create_response_from_buffer (strlen (page), (void *) page, MHD_RESPMEM_PERSISTENT);
			ret = MHD_queue_response (conn, MHD_HTTP_NOT_FOUND, response);
			MHD_destroy_response (response);

		} else {
			SyncCallCtx ctx(unit);
			ctx.exec(url);
			String text = ctx.output;
			response = MHD_create_response_from_buffer (text.size(), (char*)text.c_str(), MHD_RESPMEM_MUST_COPY);
			ret = MHD_queue_response (conn, MHD_HTTP_OK, response);
			MHD_destroy_response (response);

			printf("[%s]: ok\n",url);
		}
		return ret;
	}

	void release(){
		if ( this->daemon ){
			cerr << "[Httpd Control]: release;\n";
			MHD_stop_daemon (this->daemon);
			this->daemon = NULL;
		}
	}
};






extern "C" {
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/imgutils.h>
	#include <libswscale/swscale.h>
}


struct RtmpCtrl {

	RtmpCtrl(){

	}

	void initialize_avformat_context(AVFormatContext *&fctx, const char *format_name){
		int ret = avformat_alloc_output_context2(&fctx, NULL, format_name, NULL);
		if (ret < 0){
			std::cout << "Could not allocate output format context!" << std::endl;
			exit(1);
		}
	}

	void initialize_io_context(AVFormatContext *&fctx, const char *output){
		if (!(fctx->oformat->flags & AVFMT_NOFILE)) {
			int ret = avio_open2(&fctx->pb, output, AVIO_FLAG_WRITE, NULL, NULL);
			if (ret < 0) {
				std::cout << "Could not open output IO context!" << std::endl;
				exit(1);
			}
		}
	}


	void set_codec_params(AVFormatContext *&fctx, AVCodecContext *&codec_ctx, double width, double height, int fps, int bitrate){
		const AVRational dst_fps = {fps, 1};
		codec_ctx->codec_tag = 0;
		codec_ctx->codec_id = AV_CODEC_ID_H264;
		codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
		codec_ctx->width = width;
		codec_ctx->height = height;
		codec_ctx->gop_size = 12;
		codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
		codec_ctx->framerate = dst_fps;
		codec_ctx->time_base = av_inv_q(dst_fps);
		codec_ctx->bit_rate = bitrate;
		if (fctx->oformat->flags & AVFMT_GLOBALHEADER) {
			codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
	}


	void initialize_codec_stream(AVStream* stream, AVCodecContext* codec_ctx, AVCodec* codec, std::string codec_profile) {
		int ret;
		/*ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
		if (ret < 0) {
			std::cout << "Could not initialize stream codec parameters!" << std::endl;
			exit(1);
		}* /

		AVDictionary *codec_options = NULL;
		av_dict_set(&codec_options, "profile", codec_profile.c_str(), 0);
		av_dict_set(&codec_options, "preset", "superfast", 0);
		av_dict_set(&codec_options, "tune", "zerolatency", 0);

		// open video encoder
		ret = avcodec_open2(codec_ctx, codec, &codec_options);
		if (ret < 0) {
			std::cout << "Could not open video encoder!" << std::endl;
			exit(1);
		}
	}




	void init(){
		int cameraID = 0, fps = 30, width = 800, height = 600, bitrate = 300000;

		#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
			av_register_all();
		#endif
		avformat_network_init();

		String codec_profile = "high444";
		String server = "rtmp://localhost/live/stream";

		int ret;
		const char *output = server.c_str();

		//int height = 576;
		//int width  = 704;
		std::vector<uint8_t> imgbuf(height * width * 3 + 16);
		cv::Mat image(height, width, CV_8UC3, imgbuf.data(), width * 3);

		AVFormatContext *ofmt_ctx = NULL;
		initialize_avformat_context(ofmt_ctx, "flv");
		initialize_io_context(ofmt_ctx, server.c_str());

		AVCodec* out_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		AVStream* out_stream = avformat_new_stream(ofmt_ctx, out_codec);
		AVCodecContext* out_codec_ctx = avcodec_alloc_context3(out_codec);

		set_codec_params(ofmt_ctx, out_codec_ctx, width, height, fps, bitrate);

		initialize_codec_stream(out_stream, out_codec_ctx, out_codec, codec_profile);

		out_stream->codecpar->extradata = out_codec_ctx->extradata;
		out_stream->codecpar->extradata_size = out_codec_ctx->extradata_size;
		av_dump_format(ofmt_ctx, 0, output, 1);

		//auto cam = get_device(camID, width, height);
		/*



		auto *swsctx = initialize_sample_scaler(out_codec_ctx, width, height);
		auto *frame = allocate_frame_buffer(out_codec_ctx, width, height);

		int cur_size;
		uint8_t *cur_ptr;
	* /

		ret = avformat_write_header(ofmt_ctx, nullptr);
		if (ret < 0){
			std::cout << "Could not write header!" << std::endl;
			exit(1);
		}

		bool end_of_stream = false;
		do {
			cam >> image;
			const int stride[] = {static_cast<int>(image.step[0])};
			sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
			frame->pts += av_rescale_q(1, out_codec_ctx->time_base, out_stream->time_base);
			write_frame(out_codec_ctx, ofmt_ctx, frame);
		} while (!end_of_stream);

		av_write_trailer(ofmt_ctx);
		av_frame_free(&frame);
		avcodec_close(out_codec_ctx);
		avio_close(ofmt_ctx->pb);
		avformat_free_context(ofmt_ctx);
	}


};*/







Record record;
//HttpCtrl ctrl(8080);

void signal_callback(int sig){
	record.release();
	exit(0);
}


int main(){
	signal(SIGINT, signal_callback);
	while ( true ){
		record.execute();
	}
	return 0;
}
