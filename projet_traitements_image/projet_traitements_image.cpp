// projet_traitements_image.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// The "Square Detector" program.
// It loads several images sequentially and tries to find squares in
// each image

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <iostream>
#include <fstream>
#include <cmath>
#include <map>
#include <string>
#include <string.h>
#include <cstdio>
#include <numeric>
#include <regex>
#define GET_NAME(variable) (#variable)


using namespace std;

static void help()
{
	cout <<
		"\nA program using pyramid scaling, Canny, contours, contour simpification and\n"
		"memory storage to find squares in a list of images\n"
		"Returns sequence of squares detected on the image.\n"
		"the sequence is stored in the specified memory storage\n"
		"Call:\n"
		"./squares\n"
		"Using OpenCV version %s\n" << CV_VERSION << "\n" << endl;
}


int thresh = 50, N = 5;
const char* wndname = "Square Detection Demo";

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
static double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);
}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage
static void findSquares(const cv::Mat& image, vector<vector<cv::Point>>& squares)
{
	squares.clear();

	//s    cv::Mat pyr, timg, gray0(image.size(), CV_8U), gray;

	// down-scale and upscale the image to filter out the noise
	//pyrDown(image, pyr, Size(image.cols/2, image.rows/2));
	//pyrUp(pyr, timg, image.size());


	// blur will enhance edge detection

	cv::Mat timg(image);
	//medianBlur(image, timg, 9);
	cv::Mat gray0(timg.size(), CV_8U), gray;

	vector<vector<cv::Point>> contours;

	// find squares in every color plane of the image
	for (int c = 0; c < 3; c++)
	{
		int ch[] = {c, 0};
		mixChannels(&timg, 1, &gray0, 1, ch, 1);

		// try several threshold levels
		for (int l = 0; l < N; l++)
		{
			// hack: use Canny instead of zero threshold level.
			// Canny helps to catch squares with gradient shading
			if (l == 0)
			{
				// apply Canny. Take the upper threshold from slider
				// and set the lower to 0 (which forces edges merging)
				Canny(gray0, gray, 5, thresh, 5);
				// dilate canny output to remove potential
				// holes between edge segments
				dilate(gray, gray, cv::Mat(), cv::Point(-1, -1));
			}
			else
			{
				// apply threshold if l!=0:
				//     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
				gray = gray0 >= (l + 1) * 255 / N;
			}

			// find contours and store them all as a list
			findContours(gray, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

			vector<cv::Point> approx;

			// test each contour
			for (size_t i = 0; i < contours.size(); i++)
			{
				// approximate contour with accuracy proportional
				// to the contour perimeter
				approxPolyDP(cv::Mat(contours[i]), approx, arcLength(cv::Mat(contours[i]), true) * 0.02, true);

				// square contours should have 4 vertices after approximation
				// relatively large area (to filter out noisy contours)
				// and be convex.
				// Note: absolute value of an area is used because
				// area may be positive or negative - in accordance with the
				// contour orientation
				if (approx.size() == 4 &&
					fabs(contourArea(cv::Mat(approx))) > 1000 &&
					isContourConvex(cv::Mat(approx)))
				{
					double maxCosine = 0;

					for (int j = 2; j < 5; j++)
					{
						// find the maximum cosine of the angle between joint edges
						double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
						maxCosine = MAX(maxCosine, cosine);
					}

					// if cosines of all angles are small
					// (all angles are ~90 degree) then write quandrange
					// vertices to resultant sequence
					if (maxCosine < 0.3)
						squares.push_back(approx);
				}
			}
		}
	}
}


// the function draws all the squares in the image
static void drawSquares(cv::Mat& image, const vector<vector<cv::Point>>& squares, cv::Scalar color)
{
	for (size_t i = 0; i < squares.size(); i++)
	{
		const cv::Point* p = &squares[i][0];

		int n = (int)squares[i].size();
		//dont detect the border
		if (p->x > 3 && p->y > 3)
			polylines(image, &p, &n, 1, true, color, 3);
	}

	imshow(wndname, image);
}

typedef vector<cv::Point> square_t;

// returns true if cv::PointA < cv::PointB
// orders points by row first, then column
bool compare_points(cv::Point& pointA, cv::Point& pointB, float proximity_tolerance)
{
	if (pointA.y < pointB.y - proximity_tolerance) return true;
	if (pointA.y > pointB.y + proximity_tolerance) return false;
	if (pointA.x < pointB.x - proximity_tolerance) return true;
	return false;
}

// returns true if the upper left corner of a is < ulc of b
bool compare_square_t(square_t& a, square_t& b)
{
	cv::Point pointA = a[0], pointB = b[0];
	return compare_points(pointA, pointB, 50);
}

int upperLeft(square_t& sq)
{
	int idx = 0;
	cv::Point min = sq[0];
	for (int i = 1; i < sq.size(); i++)
	{
		if (sq[i].x <= min.x && abs(sq[i].y - min.y) < 20)
		{
			min = sq[i];
			idx = i;
		}
	}
	return idx;
}


void printSquares(vector<square_t>& squares, string filename)
{
	ofstream myfile2;
	myfile2.open("C:/Users/sbeaulie/Desktop/" + filename);

	//Le rotate ne marchait pas
	for (square_t& sq : squares)
	{
		for (int i = 0; i < sq.size(); i++)
		{
			myfile2 << sq[i] << endl;
		}
		myfile2 << endl;
	}

	myfile2.close();
}

void filterBySize(vector<square_t>& in, vector<square_t>& out)
{
	for (int i = 0; i < in.size(); i++)
	{
		//def des points
		cv::Point p1 = in[i][0];
		cv::Point p2 = in[i][1];
		cv::Point p3 = in[i][2];
		cv::Point p4 = in[i][3];

		//calcul de taille du carré
		int distancex = (p2.x - p1.x) ^ 2;
		int distancey = (p2.y - p1.y) ^ 2;

		double width = sqrt(abs(distancex - distancey));

		if (width > 15.5 && width < 17)
		{
			vector<cv::Point> points;
			points.push_back(p1);
			points.push_back(p2);
			points.push_back(p3);
			points.push_back(p4);
			out.push_back(points);
		}
	}
}

// rotate each square so that the upper left corner is the first cv::Point
void rotateSquares(vector<square_t>& squaresBis)
{
	for (square_t& sq : squaresBis)
	{
		int mouv = upperLeft(sq);
		vector<cv::Point> inter = sq;
		sq[(0 + mouv) % 4] = inter[0];
		sq[(1 + mouv) % 4] = inter[1];
		sq[(2 + mouv) % 4] = inter[2];
		sq[(3 + mouv) % 4] = inter[3];
	}
}


bool squaresOverlap(square_t& a, square_t& b, float tolX, float tolY)
{
	cv::Point pointA = a[0], pointB = b[0];
	return abs(pointB.x - pointA.x) < 160 && abs(pointB.y - pointA.y) < 320;
}

void filterOverlappingSquares(vector<square_t>& in, vector<square_t>& out, float tolX, float tolY)
{
	std::sort(in.begin(), in.end(), compare_square_t);

	for (auto sq : in)
	{
		if (out.size() == 0
			|| !squaresOverlap(out[out.size() - 1], sq, tolX, tolY))
		{
			out.push_back(sq);
		}
	}
}


bool areSameRow(square_t a, square_t b, float tolY)
{
	return std::abs(a[0].y - b[0].y) < tolY;
}

vector<vector<square_t>> groupByRow(vector<square_t>& squares)
{
	//Trier les carrés par lignes
	vector<vector<square_t>> lignes;

	std::sort(squares.begin(), squares.end(), compare_square_t);

	vector<square_t> currentRow;

	currentRow.push_back(squares[0]);
	for (int i = 1; i < squares.size(); i++)
	{
		if (!areSameRow(squares[i], squares[i - 1], 160))
		{
			lignes.push_back(currentRow);
			currentRow = vector<square_t>();
		}
		currentRow.push_back(squares[i]);
	}
	lignes.push_back(currentRow);
	return lignes;
}


//charge la base de template // TODO clean that up
static map<string, cv::Mat> base;


static const string template_names[] = {
	"accident",
	"bomb",
	"car",
	"casualty",
	"electricity",
	"fire",
	"gas",
	"injury",
	"paramedics",
	"person",
	"police",
	"roadBlock",
	"small",
	"medium",
	"large",
};

map<string, cv::Mat> loadTemplates()
{
	const string img_prefix = "C:/Users/clifr/Documents/Git/projet_traitement_image/images/templates/";

	map<string, cv::Mat> result;

	for (const auto name : template_names)
	{
		result.insert_or_assign(name, cv::imread(img_prefix + name + ".png"));
	}

	return result;
}


string* whatSymbols(cv::Mat source)
{
	double max_result = 0.0;
	string symbol_name;
	int indice = 0;


	cv::Mat bestResult;
	// Symbole le plus ressemblant
	for (int i = 0; i < base.size(); i++)
	{
		cv::Mat result;
		matchTemplate(source, base[template_names[i]], result, CV_TM_CCOEFF_NORMED);
		//permet la récupération du point d'intérêt (haut a gauche) le plus probable
		double min, max;
		cv::Point locationMin;
		cv::Point locationMax;
		minMaxLoc(result, &min, &max, &locationMin, &locationMax);

		if (max > max_result)
		{
			max_result = max;
			indice = i;
			bestResult = result;
		}
	}
	//	cv::namedWindow("res", cv::WINDOW_NORMAL);
	//	cv::imshow("res", bestResult);


	switch (indice)
	{
	case 0: symbol_name = "accident";
		break;
	case 1: symbol_name = "bomb";
		break;
	case 2: symbol_name = "car";
		break;
	case 3: symbol_name = "casualty";
		break;
	case 4: symbol_name = "electricity";
		break;
	case 5: symbol_name = "fire";
		break;
	case 6: symbol_name = "fireBrigade";
		break;
	case 7: symbol_name = "flood";
		break;
	case 8: symbol_name = "gas";
		break;
	case 9: symbol_name = "injury";
		break;
	case 10: symbol_name = "paramedics";
		break;
	case 11: symbol_name = "person";
		break;
	case 12: symbol_name = "police";
		break;
	case 13: symbol_name = "roadBlock";
		break;
	}

	// taille de l'image

	double maxResult1 = 0.0;
	string symbolTaille;
	int couleur = 0;
	// Symbole le plus ressemblant
	for (int i = 0; i < 3; i++)
	{
		cv::Mat result;
		cv::Mat choix;
		if (i == 0)
		{
			choix = base["small"];
		}
		else if (i == 1)
		{
			choix = base["medium"];
		}
		else
		{
			choix = base["large"];
		}
		matchTemplate(source, choix, result, CV_TM_CCOEFF_NORMED);

		//permet la récupération du point d'intérêt (haut a gauche) le plus probable
		double min, max;
		cv::Point locationMin;
		cv::Point locationMax;
		minMaxLoc(result, &min, &max, &locationMin, &locationMax);
		if (max > maxResult1)
		{
			maxResult1 = max;
			couleur = i;
		}
	}
	switch (couleur)
	{
	case 0: symbolTaille = "small";
		break;
	case 1: symbolTaille = "medium";
		break;
	case 2: symbolTaille = "large";
		break;
	}

	return new string[2]{symbol_name, symbolTaille};
}


string getFileName(string numRow, string numCol, string numScripteur, string numPage, string templateName,
                   string templateSize)
{
	return templateName + "_" + numScripteur + "_" + numPage + "_" + numRow + "_" + numCol;
}


string* parseInputName(string filePath)
{
	std::regex rgx(".*/w(\\d\\d\\d)-scans/(.+).png");
	std::smatch matches;

	if (std::regex_match(filePath, matches, rgx))
	{
		return new string[2]{matches[1], matches[2]};
	}
	else
	{
		throw "Incorrect input filename\n";
	}
}

int main(const int argc, char** argv)
{
	if (argc < 1)
	{
		std::cout << "invalid parameters";
		std::exit(-1);
	}
	string img_path = argv[0];

	string* parsed = parseInputName(img_path); // TODO parse parameters
	string scripterNumber = parsed[0];
	string pageNumber = parsed[1];

	string ComputedImagesPrefix = "C:/Users/sbeaulie/Desktop/ComputedImages/";

	base = loadTemplates();

	//	char* cstr = new char[img_path.length() + 1];
	//	strcpy(cstr, img_path.c_str());

	//	static const char* names[] = {cstr , 0};

	help();
	cv::namedWindow(wndname, cv::WINDOW_NORMAL);
	vector<square_t> squares;

	//	for (int i = 0; names[i] != 0; i++)
	//	{
	cv::Mat image = cv::imread(img_path, 1);
	if (image.empty())
	{
		cout << "Couldn't load " << img_path << endl;
		return -1;
	}

	findSquares(image, squares);


	vector<square_t> squaresBis;
	filterBySize(squares, squaresBis);
	rotateSquares(squaresBis);

	vector<square_t> filtered;
	filterOverlappingSquares(squaresBis, filtered, 160, 320);

	cout << filtered.size() << endl;

	//drawSquares(image, filtered);

	auto lignes = groupByRow(filtered);

	drawSquares(image, lignes[2], cv::Scalar(0, 255, 0));


	for (int k = 0; k < lignes.size(); k++)
	{
		//Select interest zone 
		const cv::Mat source = cv::imread(img_path);
		const cv::Mat sub_image(source, cv::Rect(0, lignes[k][0][0].y, 600, 350));

		string* templateAndSize = whatSymbols(sub_image);
		string numberRow = to_string(k + 1);

		for (int u = 0; u < lignes[k].size(); u++)
		{
			string numberColumn = to_string(u + 1);

			// Cropped square
			cv::Mat cropped(source, cv::Rect(lignes[k][u][0], lignes[k][u][2]));

			auto filename = getFileName(numberRow, numberColumn, scripterNumber, pageNumber, templateAndSize[0],
			                            templateAndSize[1]);

			cout << ComputedImagesPrefix + filename + ".png" << endl;

			cv::imwrite(ComputedImagesPrefix + filename + ".png", cropped);

			//String pour .txt


			ofstream metadataFile;
			metadataFile.open(ComputedImagesPrefix + filename + ".txt");
			metadataFile << "# 2017 Groupe Beaulieu Fournier Saulnier\n"
				<< "label " << templateAndSize[0] << endl
				<< "form " << scripterNumber + pageNumber << endl
				<< "scripter " << scripterNumber << endl
				<< "page " << pageNumber << endl
				<< "row " << numberRow << endl
				<< "column " << numberColumn << endl
				<< "size " << templateAndSize[1] << endl;


			metadataFile.close();
		}
		//		}
//endLoop:


		int c = cv::waitKey();
		if (static_cast<char>(c) == 27)
			break;
	}
	return 0;
}

/*
Problèmes :
-Carrés en trop
-Récupérer numéro de page
-Peutêtre une lignes dans "lignes" de trop
*/
