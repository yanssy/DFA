/*
 *  geneFace.cpp
 *
 *  Created by Shunsuke Saito on 3/26/14.
 *  Copyright 2010 Shunsuke Saito. All rights reserved.
 *
 */

#include "geneFace.h"
#include "movingPatch.h"

void GENEFACE::makeIntermFrame(const cv::Mat &imgA,const cv::Mat &imgB,const int frameNum,vector<cv::Mat> &vImage)
{
	cv::Mat imageA_Lab;
	cv::Mat imageB_Lab;
	cv::cvtColor(imgA,imageA_Lab,cv::COLOR_BGR2Lab);
	cv::cvtColor(imgB,imageB_Lab,cv::COLOR_BGR2Lab);

	vector<cv::Mat> vImageA;
	vector<cv::Mat> vImageB;

	cv::buildPyramidAnySize(imageA_Lab,vImageA,scaleDown,0.83);
	cv::buildPyramidAnySize(imageB_Lab,vImageB,scaleDown,0.83);
	
	//Initailize 
	vector<cv::Mat> vFrame;
	movingPatch(15,frameNum,vImageA[scaleDown],vImageB[scaleDown],vFrame);

	for(int i=0;i<frameNum;++i)
	{
		stringstream test;
		test << i << ".jpg";
		cv::Mat temp;
		cv::cvtColor(vFrame[i],temp,cv::COLOR_Lab2BGR);
		cv::imwrite(test.str(),temp);
	}
	cv::waitKey(5);

	int maxIterate=1;
	for(int a=0;a<=scaleDown;++a)
	{
		cout << "Scale: " << a << endl;
		for(int b=0;b<maxIterate;++b)
		{
			cout << "Iteration: " << b << endl;
			cout << "Frame: ";
			for(int i=1;i<frameNum-1;++i)
			{
				cout << i << " ";
				double alpha=(frameNum-i)/(double)frameNum;

				//Compute T3
				cout << "Computing T3" << endl;
				cv::Mat T3=vFrame[i].clone();
				minimizeDistComp(5,0,vFrame[0],T3);

				//Compute T4
				cout << "Computing T4" << endl;
				cv::Mat T4=vFrame[i].clone();
				minimizeDistComp(5,0,vFrame[frameNum-1],T4);

				//Compute T5
				cout << "Computing T5" << endl;
				cv::Mat T5=vFrame[i].clone();
				minimizeDAlphaDisCohere(patchSize,0,alpha,vFrame[0],vFrame[frameNum-1],T5);

				cv::Mat temp;
				cv::cvtColor(T3,temp,cv::COLOR_Lab2BGR);
				cv::imwrite("T3.jpg",temp);
				cv::cvtColor(T4,temp,cv::COLOR_Lab2BGR);
				cv::imwrite("T4.jpg",temp);
				cv::cvtColor(T5,temp,cv::COLOR_Lab2BGR);
				cv::imwrite("T5.jpg",temp);

				//Add T1~5
				vFrame[i]=beta*alpha*T3+beta*(1.0-alpha)*T4+(1.0-beta)*T5;
			}
			cout << endl;
		}

		if(a!=scaleDown){
			//Upscaling
			for(int c=1;c<frameNum-1;++c)
			{
				cv::resize(vFrame[c],vFrame[c],vImageA[scaleDown-a-1].size(),cv::INTER_LANCZOS4);
			}
			vFrame[0]=vImageA[scaleDown-a-1].clone(); 
			vFrame[frameNum-1]=vImageB[scaleDown-a-1].clone();
			maxIterate=6*(scaleDown-a-1)/scaleDown+2;
		}
	}
	cout << "Save Image..." << endl;
	//Save Image
	vImage.resize(frameNum-2);
	for(int i=1;i<frameNum-1;++i)
	{
		cv::cvtColor(vFrame[i],vFrame[i],cv::COLOR_Lab2BGR);
		vImage[i-1]=vFrame[i];
	}
}

void GENEFACE::makeSentense(const string &output)
{
	miniDist.minimizeDistortion();

	cv::Size imgSize((int)(backMovie.get(CV_CAP_PROP_FRAME_WIDTH)),(int)(backMovie.get(CV_CAP_PROP_FRAME_HEIGHT)));
	cv::VideoWriter outVideo(output,static_cast<int>(backMovie.get(CV_CAP_PROP_FOURCC)),static_cast<int>(backMovie.get(CV_CAP_PROP_FPS)),imgSize,true);
	

	bool inInterp=false;
	int frameSize=0;
	cv::Mat frameA,frameB;

	for(int i=0;i<miniDist.result.size();++i)
	{
		int duration=miniDist.result[i].duration;
		if(duration<3){
			if(inInterp){
				frameSize+=duration;
			}
			else{
				for(int j=0;j<duration;++j)
				{
					cv::Mat frame;
					backMovie.set(CV_CAP_PROP_POS_FRAMES,(double)miniDist.result[i].startFrame+j*(duration/(double)miniDist.result[i].actualSize));
					backMovie >> frame;
					outVideo << frame;
				}
			}
		}

		for(int j=0;j<duration;++j)
		{
			++frameSize;

			if(j==1&&duration==3){ 
				
				backMovie.set(CV_CAP_PROP_POS_FRAMES,(double)miniDist.result[i].startFrame+2);
				backMovie >> frameB;
				if(i!=0){
					std::vector<cv::Mat> vImage;
					makeIntermFrame(frameA,frameB,frameSize,vImage);
					for(int k=0;k<vImage.size();++k)
					{
						outVideo << vImage[k];
					}
					outVideo << frameB;
				}
				frameA=frameB.clone();
				frameSize=1;
			}

			if(i!=0&&j==2){
				backMovie.set(CV_CAP_PROP_POS_FRAMES,(double)miniDist.result[i].startFrame+2);
				backMovie >> frameB;
				std::vector<cv::Mat> vImage;
				makeIntermFrame(frameA,frameB,frameSize,vImage);
				for(int k=0;k<vImage.size();++k)
				{
					outVideo << vImage[k];
				}
				outVideo << frameB;
				inInterp=false;
			}
			if(j==duration-3){
				backMovie.set(CV_CAP_PROP_POS_FRAMES,(double)miniDist.result[i].endFrame-2);
				backMovie >> frameA;
				outVideo << frameA;
				inInterp=true;
				frameSize=1;
			}

			if(!inInterp){
				cv::Mat frame;
				backMovie.set(CV_CAP_PROP_POS_FRAMES,(double)miniDist.result[i].startFrame+j*(duration/(double)miniDist.result[i].actualSize));
				backMovie >> frame;
				outVideo << frame;
			}
		}

	}
}