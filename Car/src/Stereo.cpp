#include "../header/includeFiles.h"
#include "../header/Stereo.h"
#include "../header/Car.h"

#define DISPCHANGEMAX 235
#define DISPCHANGEMIN 130
#define NORMALISE_THRESH_MAX 0.45
#define NORMALISE_THRESH_MIN 0.25

bool brake = false;

Stereo::Stereo(int SADWindowSize)
{
    cout << "Creating Stereo object." << endl;
    maxDisp = 5;
    minDisp = 3;
    numObjects = 3;
    totalArea = 0;
    visual = FLAGS::FAR;
    stereoInfo = FLAGS::NORMAL;
    visualHistoryIndex = 0;
    normaliseThresh = NORMALISE_THRESH_MAX;

    Initialise(minDisp, SADWindowSize);
}

void Stereo::Initialise(int disp, int SADWindowSize)
{
    nDisparity = 16 * disp;
    sbm = StereoBM(StereoBM::BASIC_PRESET, nDisparity, SADWindowSize);
}
/*remember the frames to stop jerky movement of the car when on the boundary*/
void Stereo::addFrameToHistory()
{
    if (visualHistoryIndex == 3)
        visualHistoryIndex = 0;
    visualHistory[visualHistoryIndex] = dispChange;
    visualHistoryIndex++;
}

/*only start moving if previous three frames consistently say so*/
bool Stereo::visualChangeAllowed()
{
    if ((visualHistory[0] == FLAGS::DECREMENT)
            && (visualHistory[1] == FLAGS::DECREMENT)
            && (visualHistory[2] == FLAGS::DECREMENT))
    {
        return true;
    }
    else
    {
//        cout << "visual change not allowed " << endl;
        return false;
    }
}

bool Stereo::changeParameters(int _SADWindowSize)
{
    if (dispChange == FLAGS::UNCHANGED)
    {
        return false;
    }
    else if (dispChange == FLAGS::INCREMENT)
    {
        Initialise(maxDisp, _SADWindowSize);
        visual = FLAGS::NEAR;
        normaliseThresh = NORMALISE_THRESH_MIN;
    }
    else if (dispChange == FLAGS::DECREMENT)
    {
        if (visualChangeAllowed())
        {
            Initialise(minDisp, _SADWindowSize);
            visual = FLAGS::FAR;
            normaliseThresh = NORMALISE_THRESH_MAX;
        }
    }
    return true;
}

//To decide whether the disparity buffer needs to be discarded
bool Stereo::parameterChangeRequired()
{
    if (dispChange != FLAGS::UNCHANGED)
        return true;
    else
        return false;
}

Mat Stereo::disparityMap(StereoPair &images)
{
    int rows = images.leftImage.rows;
    int cols = images.leftImage.cols;

    Mat imgDisparity16S = Mat(rows, cols, CV_16S);
    Mat imgDisparity8U = Mat(rows, cols, CV_8UC1);

//Calculate the disparity image
    sbm(images.leftImage, images.rightImage, imgDisparity16S, CV_16S);

//Check the extreme values
    double minVal;
    double maxVal;
    minMaxLoc(imgDisparity16S, &minVal, &maxVal);

//    cout << (255 / (maxVal - minVal)) << endl;

//    imgDisparity16S.convertTo(imgDisparity8U, CV_8UC1, 255 / (maxVal - minVal));

    imgDisparity16S.convertTo(imgDisparity8U, CV_8UC1, normaliseThresh);

    return imgDisparity8U;
}

bool Stereo::detectObjects(Mat &dispMap)
{
//    blur(dispMap, dispMap, Size(3,3));

    Mat element(3, 3, CV_8U, cv::Scalar(1));

    erode(dispMap, dispMap, element);

    IplImage *dispIpl = new IplImage(dispMap); //create an IplImage from Mat

//Declare variables
    CBlobResult blobs;
    int minArea = 250;

    blobs = CBlobResult(dispIpl, NULL, 0); //get all blobs in the disparity map
    blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, minArea); //filter blobs by area and remove all less than minArea

    numObjects = blobs.GetNumBlobs();

    CBlob* currentBlob;
    vector<int> meanPixelValues;
    vector<double> areas; //store all blob areas

    totalArea = 0; //reset value, the other ones get overwritten, this one gets added to
    boundingBoxes.clear();

//    Mat displayColor;
//    dispMap.convertTo(displayColor, CV_32F, 1. / 255.);
//    cv::cvtColor(displayColor, displayColor, CV_GRAY2RGB);

    for (int i = 0; i < numObjects; i++)
    {
        currentBlob = blobs.GetBlob(i);
        meanPixelValues.push_back(currentBlob->Mean(dispIpl));

        boundingBoxes.push_back(currentBlob->GetBoundingBox());
        totalArea += currentBlob->Area();
        areas.push_back(currentBlob->Area());
    }

    vector<int>::const_iterator it;

    if (meanPixelValues.size() > 0)
    {
//        sort(meanPixelValues.begin(), meanPixelValues.end());

        it = max_element(meanPixelValues.begin(), meanPixelValues.end());
        closestObjectVal = *it;
        objectBoundingBox = boundingBoxes[it - meanPixelValues.begin()]; //copy variable to return (it will go out of scope otherwise)

        objArea = areas[it - meanPixelValues.begin()];
    }
    else
    {
        closestObjectVal = -1;
        objArea = -1;
        totalArea = -1;
        return false;
    }

//    int x = objectBoundingBox.x;
//    int y = objectBoundingBox.y;
//    int width = objectBoundingBox.width;
//    int height = objectBoundingBox.height;
//
//    rectangle(displayColor, Point(x, y), Point(x + width, y + height),
//            Scalar(0, 255, 255));
//
//
//    imshow("dispColor", displayColor);
//    waitKey(50);

    evaluate();
    return true;
}

void Stereo::evaluate()
{
    if (closestObjectVal > DISPCHANGEMAX)
    {
        dispChange = FLAGS::INCREMENT;
    }
    else if (closestObjectVal < DISPCHANGEMIN)
    {
        dispChange = FLAGS::DECREMENT;
    }
    else
    {
        dispChange = FLAGS::UNCHANGED;
    }
    addFrameToHistory();
}

bool Stereo::shouldBrake()
{
    if (visual == FLAGS::NEAR)
    {
        stereoInfo = FLAGS::NORMAL;
        return true;
    }
    if (numObjects == 0) //if no objects, complain
    {
//        cout << "No object. Braking." << endl;
        stereoInfo = FLAGS::NO_OBJECT;
        return true;
    }
//    if (numObjects < 2 && totalArea < 6500) //textureless object
//    {
//        cout << "Textureless. Braking" << endl;
//        stereoInfo = FLAGS::TEXTURELESS;
//        return true;
//    }
//    if (totalArea < 6000) //roughly 1/6 of total possible
//    {
//        cout << "Too close. Braking" << endl;
//        stereoInfo = FLAGS::TOO_CLOSE;
//        return true;
//    }
    else
        return false;
}

int Stereo::getNumObjects()
{
    return numObjects;
}

/* Must call detectObjects before this *
 * Returns the average pixel value of object in disparity map*/
int Stereo::getClosestObjectVal()
{
    return closestObjectVal;
}

int Stereo::getClosestObjectArea()
{
    return objArea;
}

int Stereo::getTotalArea()
{
    return totalArea;
}

FLAGS::VISUALS Stereo::getVisualInfo()
{
    return visual;
}
