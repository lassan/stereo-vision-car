#include "../header/main.h"

/*Variables for synchronising workers*/
bool _buffer0Processed = true;
bool _buffer1Processed = true;
bool _invalidateDispBufLeft = false;
bool _invalidateDispBufRight = false;

bool _serverEnabled = false; //true: requires client availability

string _messageToSend = "";
bool _override = true;

Server _server;
Stereo _stereo(21);
Car _car;

void Initialise()
{
    omp_set_num_threads(OMPTHREADS);

    if (_serverEnabled)
    {
        try
        {
            _server.initialise();
        } catch (string& err)
        {
            cerr << err << endl;
            cerr << "Trying to connect again: " << endl;
        }
    }

    /*Load calibration matrices from file*/
    cout << "Loading camera calibration data" << endl;
    InitCalibrationData();

    /*Open camera streams*/
    cout << "Initialising stereo cameras" << endl;
    InitCameras();

    cout << "Filling buffers" << endl;
    InitBuffers();
    cout << "Good to go" << endl;
}

void InitCalibrationData()
{
    string filePath = "CalibrationMatrices320/";
    LoadMatrixFromFile(filePath, "M1", _M1);
    LoadMatrixFromFile(filePath, "D1", _D1);
    LoadMatrixFromFile(filePath, "M2", _M2);
    LoadMatrixFromFile(filePath, "D2", _D2);
    LoadMatrixFromFile(filePath, "R1", _R1);
    LoadMatrixFromFile(filePath, "R2", _R2);
    LoadMatrixFromFile(filePath, "P1", _P1);
    LoadMatrixFromFile(filePath, "P2", _P2);
    LoadMatrixFromFile(filePath, "Q", _Q);
}

void InitCameras()
{
    /*Create rectification matrices*/
    initUndistortRectifyMap(_M1, _D1, _R1, _P1, Size(320, 240), CV_16SC2,
            _leftCameraMap1, _leftCameraMap2);
    initUndistortRectifyMap(_M2, _D2, _R2, _P2, Size(320, 240), CV_16SC2,
            _rightCameraMap1, _rightCameraMap2);

    _leftCamera.open(2);
    _rightCamera.open(1);

    if (_leftCamera.isOpened())
    {
        cout << "Left camera stream opened" << endl;
    }
    else
    {
        cout << "Left  camera stream failed to open. Terminating" << endl;
        abort();
    }

    if (_rightCamera.isOpened())
    {
        cout << "Right camera stream opened" << endl;
    }
    else
    {
        cout << "Right camera stream failed to open. Terminating" << endl;
        abort();
    }

    _leftCamera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    _leftCamera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

    _rightCamera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
    _rightCamera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
}

void InitBuffers()
{
    GetStereoImages(_buffer0);
    GetStereoImages(_buffer1);

    _disparityBuffer.leftImage = _stereo.disparityMap(_buffer0);
    _disparityBuffer.rightImage = _stereo.disparityMap(_buffer1);
}

void GetStereoImages(StereoPair &input)
{
    _leftCamera.grab();
    _rightCamera.grab();

    _leftCamera.retrieve(input.leftImage, 0);
    _rightCamera.retrieve(input.rightImage, 0);

    remap(input.leftImage, input.leftImage, _leftCameraMap1, _leftCameraMap2,
            INTER_LINEAR);
    remap(input.rightImage, input.rightImage, _rightCameraMap1,
            _rightCameraMap2, INTER_LINEAR);

    Rect _imageSegment(40, 60, 240, 120);

    input.leftImage = input.leftImage(_imageSegment);
    input.rightImage = input.rightImage(_imageSegment);

    cvtColor(input.leftImage, input.leftImage, CV_RGB2GRAY);
    cvtColor(input.rightImage, input.rightImage, CV_RGB2GRAY);

    blur(input.leftImage, input.leftImage, Size(3, 3));
    blur(input.rightImage, input.rightImage, Size(3, 3));
}

void printInfo()
{
    cout << "highest: " << _stereo.getClosestObjectVal() << "\tnumber: "
            << _stereo.getNumObjects() << "\tarea: " << _stereo.getTotalArea()
            << endl;
}

void ImageAcquisitionWorker()
{
    int iterationCounter = 0;
    float iterationTime = 0;
    float totalTime = 0;
    int prevFps = 0;

    while (true)
    {
        iterationTime = getTickCount();
#pragma omp critical(buffer0)
        {
            if (_invalidateDispBufLeft) //if parameter change required - ignore this buffer
            {
                _invalidateDispBufLeft = false;
            }
            else //otherwise detect objects, brake if required, and check if next buffer should be ignored
            {
                _stereo.detectObjects(_disparityBuffer.leftImage);

                controlCar();

                if (_serverEnabled)
                    _server.sendData(_buffer0, _disparityBuffer.leftImage,
                            _stereo.shouldBrake(), _car, intToString(prevFps));

                if (_stereo.parameterChangeRequired())
                    _invalidateDispBufRight = true;

//                imshow("original", _buffer0.leftImage);
//                waitKey(10);
//                imshow("disp", _disparityBuffer.leftImage);
//                waitKey(10);
            }

            GetStereoImages(_buffer0);

            iterationCounter++;

//               printInfo();
        }
#pragma omp critical(buffer1)
        {
            if (_invalidateDispBufRight) //if parameter change required - ignore this buffer
            {
                _invalidateDispBufRight = false;
            }
            else //otherwise detect objects, brake if required, and check if next buffer should be ignored
            {
                _stereo.detectObjects(_disparityBuffer.rightImage);

                controlCar();

                if (_serverEnabled)
                    _server.sendData(_buffer1, _disparityBuffer.rightImage,
                            _stereo.shouldBrake(), _car, intToString(prevFps));

                if (_stereo.parameterChangeRequired())
                    _invalidateDispBufLeft = true;
            }

            GetStereoImages(_buffer1);

            iterationCounter++;

//                printInfo();
        }

        iterationTime = (getTickCount() - iterationTime) * 0.000000001;
        totalTime += iterationTime;
        if (iterationCounter == MAX_TIMING_ITERATIONS)
        {
            prevFps = (iterationCounter) / totalTime;
            cout << prevFps <<  " fps" << endl;
            iterationCounter = 0;
            totalTime = 0;
        }
    }
}

void controlCar()
{
    if (_stereo.shouldBrake())
        _car.brake();
    else
        _car.turnBrakeLightOff();
}

void DisparityCalculationWorker()
{
    while (true)
    {

#pragma omp critical(buffer1)
        {
            _stereo.changeParameters(25);
            _disparityBuffer.rightImage = _stereo.disparityMap(_buffer1);
        }
#pragma omp critical(buffer0)
        {
            _stereo.changeParameters(25);
            _disparityBuffer.leftImage = _stereo.disparityMap(_buffer0);
        }
    }
}

int main()
{
    Initialise();

#pragma omp parallel
    {
#pragma omp sections
        {
#pragma omp section
            {
                ImageAcquisitionWorker();
            }
#pragma omp section
            {
                DisparityCalculationWorker();
            }
        }
    }
    return 0;
}

void GenerateSuperImposedImages()
{
    namedWindow("Overlaid");

    StereoPair camImages;

    GetStereoImages(camImages);

    imshow("Right image", camImages.rightImage);
    imshow("Left image", camImages.leftImage);

    Mat overlay = OverlayImages(camImages, 0.5);
    imshow("Overlaid", overlay);

    cout << "Press any key to continue." << endl;
    waitKey(0);
}
