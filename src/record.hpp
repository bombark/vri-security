#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;



struct Record {
	Mat cur_frame, old_frame, rgb_frame;//, diff;
	bool is_recording;
	VideoWriter out_video;
	VideoCapture capture;
	int record_time;

	Record(){
		this->is_recording=false;
		this->capture.open(0);
		if ( ! this->capture.isOpened() ){
			cerr << "Nao foi possivel abrir a camera\n";
			exit(1);
		}
		this->readNextFrame();
		Size size = rgb_frame.size();
		int fourcc = CV_FOURCC('U', '2', '6', '3');

		string file = "out-";
		file += this->currentDateTime();
		file += ".avi";
		out_video.open(file, fourcc, 12, Size(704,576), true);
		if ( ! this->out_video.isOpened() ){
			cerr << "Nao foi possivel abrir o video de saida\n";
			exit(1);
		}
	}


	~Record(){
		this->release();
	}

	void execute(){
		cur_frame.copyTo(old_frame);
		if ( is_recording ){
			waitKey(1000/12);
			this->readNextFrame();
			this->exec_recording();
		} else {
			waitKey(1000/2);
			this->readNextFrame();
			this->exec_stand_by();
		}
		imshow("opa4",cur_frame);
		//imshow("opa222",diff);
	}

	void readNextFrame(){
		capture >> rgb_frame;
		Size size = rgb_frame.size();
		cvtColor(rgb_frame, cur_frame, cv::COLOR_RGB2GRAY);
		resize(cur_frame, cur_frame, Size( size.width/4, size.height/4 ) );
		//return cur_frame;
	}


	void exec_recording(){
		if( this->record_time > 24 ){
			this->setStandbyState();
			return ;
		}
		size_t diff = this->calcDiff();
		if ( this->hasMovement(diff) ){
			this->record_time = 0;
		} else {
			this->record_time += 1;
		}

		 std::string date = this->currentDateTime();
		Point pt( rgb_frame.cols-250, rgb_frame.rows-32 );
		putText(rgb_frame, date, pt, 0, 0.6, Scalar(255,0,0), 1 );

		imshow("rec",rgb_frame);
		printf("recording[%s]: %6lu,%d;\n",date.c_str(),diff,record_time);

		Mat to_save;
		cv::resize( rgb_frame, to_save, Size(704,576) );
		out_video << to_save;
	}


	void exec_stand_by(){
		size_t diff = this->calcDiff();
		cout << "stand_by: " << diff << endl;
		if ( this->hasMovement(diff) ){
			this->setRecodingState();
		}
	}

	void setRecodingState(){
		this->record_time = 0;
		this->is_recording = true;
	}

	void setStandbyState(){
		this->is_recording = false;
	}

	bool hasMovement(size_t diff){
		return diff > 40;
	}

	size_t calcDiff(){
		size_t sum=0;
		for (size_t y=0; y<cur_frame.rows; y++){
			for (size_t x=0; x<cur_frame.cols; x++){
				uchar val = abs(cur_frame.at<uchar>(y,x) - old_frame.at<uchar>(y,x));
				if ( val > 20 ){
					sum += 1;
				}
			}
		}
		return sum;
	}


	std::string currentDateTime() {
		time_t     now = time(0);
		struct tm  tstruct;
		char       buf[80];
		tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
		return buf;
	}


	void release(){
		if ( this->capture.isOpened() )
			this->capture.release();
		if ( this->out_video.isOpened() )
			this->out_video.release();
	}
};
