/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The main purpose of this program is to build time-of-flight histograms for Sample-in and Sample-out data and then, by making their ratio, extract the transmission graph.
//
//First, time-of-flight histograms are built for each of the 6 FCU detectors and then summed together. For each detector, a specific selection of runs is made, following
//an efficiency study previously performed.
//In order to account for that distinction, general Sin and Sout vectors are first defined, containing all the runs. Then, additional Sin and Sout vectors are defined,
//different for each detector, containing only the runs that are to be considered in that case. Later, during the loop over all the runs, a boolean variable is used to 
//check if the run being processed is contained in the dataset of each detector, and the corresponding histogram is filled only if it is.
//The time-of-flight histogram is filled with the tof value, subtracting the time registered by the PKUP detector and then adding a calibration value
//that is different for each detector. A cut on the amplitude of the signal (cut_a) is also applied, again different for each case.
//The ToF histograms are normalized to the number of protons, extracted from the Pulse Intensity leaf of the PKUP dataframe.
//
//In this program, a distinction is made between dedicated and parasitic bunches, building different histograms for the two. The bunch type distinction is done by
//reading the PSpulse branch.
//
//By taking the ratio of Sin and Sout histograms, the Transmission histogram is obtained. One transmission histogram is produced for dedicated bunches and one for parasitic.
//To compare them, their ratio is computed and the resulting values are grouped into a histogram. The mean and standard deviation of this last histogram quantify the 
//compatibility of the two bunch types.
//The final graph is the sum of the transmissions of the two types of bunches.
//
//Differently from some other codes in this project that are written in Python, C++ is used here because filtering all bunches to extract the tof–tgamma takes a significant
//amount of time.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "TCanvas.h"
#include "TBrowser.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TMath.h"
#include <fstream>
#include "TFrame.h"
#include "TSystem.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TFile.h"
#include "TStyle.h"
#include "TBox.h"
#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include "TLine.h"
#include "TTree.h"
#include "TBrowser.h"
#include <TStyle.h>
#include <THStack.h>
#include <TArrayD.h>
#include <iomanip>
#include <math.h>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "./config/ConfigReader.h"

using namespace std;

void Transmission_ratio_final(string cmnd_filename)
{

	// import variables from txt file
	ConfigReader cfg(cmnd_filename);

	// Data path
	string prefix = cfg.getString("prefix");
	string suffix = cfg.getString("suffix");

	// Vectors with run numbers
	vector<int> SIN = cfg.getIntVector("SIN");
	vector<int> SOUT = cfg.getIntVector("SOUT");

	vector<int> Sin_DET1 = cfg.getIntVector("Sin_DET1");
	vector<int> Sout_DET1 = cfg.getIntVector("Sout_DET1");

	vector<int> Sin_DET2 = cfg.getIntVector("Sin_DET2");
	vector<int> Sout_DET2 = cfg.getIntVector("Sout_DET2");

	vector<int> Sin_DET3 = cfg.getIntVector("Sin_DET3");
	vector<int> Sout_DET3 = cfg.getIntVector("Sout_DET3");

	vector<int> Sin_DET4 = cfg.getIntVector("Sin_DET4");
	vector<int> Sout_DET4 = cfg.getIntVector("Sout_DET4");

	vector<int> Sin_DET7 = cfg.getIntVector("Sin_DET7");
	vector<int> Sout_DET7 = cfg.getIntVector("Sout_DET7");

	vector<int> Sin_DET8 = cfg.getIntVector("Sin_DET8");
	vector<int> Sout_DET8 = cfg.getIntVector("Sout_DET8");

	// Amplitude thresholds
	float cut_a_1 = cfg.getFloat("cut_a_1", 1.0f);
	float cut_a_2 = cfg.getFloat("cut_a_2", 1.0f);
	float cut_a_3 = cfg.getFloat("cut_a_3", 1.0f);
	float cut_a_4 = cfg.getFloat("cut_a_4", 1.0f);
	float cut_a_7 = cfg.getFloat("cut_a_7", 1.0f);
	float cut_a_8 = cfg.getFloat("cut_a_8", 1.0f);

	// Calibration values
	float cal_1 = cfg.getFloat("cal_1", 1.0f);
	float cal_2 = cfg.getFloat("cal_2", 1.0f);
	float cal_3 = cfg.getFloat("cal_3", 1.0f);
	float cal_4 = cfg.getFloat("cal_4", 1.0f);
	float cal_7 = cfg.getFloat("cal_7", 1.0f);
	float cal_8 = cfg.getFloat("cal_8", 1.0f);

	cout << "Amplitude cuts:" << endl;
	cout << "cut_a_1 = " << cut_a_1 << endl;
	cout << "cut_a_2 = " << cut_a_2 << endl;
	cout << "cut_a_3 = " << cut_a_3 << endl;
	cout << "cut_a_4 = " << cut_a_4 << endl;
	cout << "cut_a_7 = " << cut_a_7 << endl;
	cout << "cut_a_8 = " << cut_a_8 << endl;

	cout << "Calibration values:" << endl;
	cout << "cal 1 = " << cal_1 << endl;
	cout << "cal 2 = " << cal_2 << endl;
	cout << "cal 3 = " << cal_3 << endl;
	cout << "cal 4 = " << cal_4 << endl;
	cout << "cal 7 = " << cal_7 << endl;
	cout << "cal 8 = " << cal_8 << endl;


	// setting up through unordered set
	auto to_set = [](const vector<int> &v)
	{ return std::unordered_set<int>(v.begin(), v.end()); }; // this is a lambda function

	const auto Sin_DET1_SET = to_set(Sin_DET1);
	const auto Sin_DET2_SET = to_set(Sin_DET2);
	const auto Sin_DET3_SET = to_set(Sin_DET3);
	const auto Sin_DET4_SET = to_set(Sin_DET4);
	const auto Sin_DET7_SET = to_set(Sin_DET7);
	const auto Sin_DET8_SET = to_set(Sin_DET8);

	const auto Sout_DET1_SET = to_set(Sout_DET1);
	const auto Sout_DET2_SET = to_set(Sout_DET2);
	const auto Sout_DET3_SET = to_set(Sout_DET3);
	const auto Sout_DET4_SET = to_set(Sout_DET4);
	const auto Sout_DET7_SET = to_set(Sout_DET7);
	const auto Sout_DET8_SET = to_set(Sout_DET8);

	
	// define the variables corresponding to the tree leaves that will be extracted
	int detn;
	double tof;
	double tflash;
	double tflash_p;
	float amp;
	int BunchNumber;
	int BunchNumberPK;
	float PulseIntensity, PulseIntensityPK;
	int PSpulse, PSpulsePK;

	// define logaritmic binning for the ToF histogram
	int N_DEC_tof = 8;
	int BinPerDecade = 50; // change here the binning
	int n_vec_tof = (int)BinPerDecade * N_DEC_tof;
	double xbins_tof[100100];
	double step_tof = (double)N_DEC_tof / n_vec_tof;
	double value_tof = 1;
	for (int filler = 0; filler <= n_vec_tof; ++filler)
	{
		value_tof = step_tof * (double)filler;
		xbins_tof[filler] = (double)pow(10., value_tof);
	}

	// define tof histograms for Sample-in, distingushing each detector and also separating dedicated and parasitic bunches
	TH1D *HSin_1_dedi = new TH1D("Sin 1 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSin_1_para = new TH1D("Sin 1 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSin_2_dedi = new TH1D("Sin 2 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSin_2_para = new TH1D("Sin 2 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSin_3_dedi = new TH1D("Sin 3 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSin_3_para = new TH1D("Sin 3 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSin_4_dedi = new TH1D("Sin 4 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSin_4_para = new TH1D("Sin 4 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSin_7_dedi = new TH1D("Sin 7 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSin_7_para = new TH1D("Sin 7 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSin_8_dedi = new TH1D("Sin 8 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSin_8_para = new TH1D("Sin 8 parasitic", "", n_vec_tof, xbins_tof);

	// define tof histograms for Sample-out, distingushing each detector and also separating dedicated and parasitic bunches
	TH1D *HSout_1_dedi = new TH1D("Sout 1 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSout_1_para = new TH1D("Sout 1 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSout_2_dedi = new TH1D("Sout 2 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSout_2_para = new TH1D("Sout 2 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSout_3_dedi = new TH1D("Sout 3 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSout_3_para = new TH1D("Sout 3 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSout_4_dedi = new TH1D("Sout 4 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSout_4_para = new TH1D("Sout 4 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSout_7_dedi = new TH1D("Sout 7 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSout_7_para = new TH1D("Sout 7 parasitic", "", n_vec_tof, xbins_tof);
	TH1D *HSout_8_dedi = new TH1D("Sout 8 dedicated", "", n_vec_tof, xbins_tof);
	TH1D *HSout_8_para = new TH1D("Sout 8 parasitic", "", n_vec_tof, xbins_tof);

	// Histograms to store the sum of the different detectors
	TH1D *HSin_DEDI = new TH1D("ToF Sin - dedicated bunches", "", n_vec_tof, xbins_tof);
	HSin_DEDI->GetXaxis()->SetTitle("ToF - Tpkup + #Delta (ns)");
	HSin_DEDI->GetYaxis()->SetTitle("Entries / N protons");
	HSin_DEDI->SetTitle("ToF dedicated bunches");
	TH1D *HSin_PARA = new TH1D("ToF Sin - parasitic bunches", "", n_vec_tof, xbins_tof);
	HSin_PARA->GetXaxis()->SetTitle("ToF - Tpkup + #Delta (ns)");
	HSin_PARA->GetYaxis()->SetTitle("Entries / N protons");
	HSin_PARA->SetTitle("ToF parasitic bunches");

	TH1D *HSout_DEDI = new TH1D("ToF Sout - dedicated bunches", "", n_vec_tof, xbins_tof);
	HSout_DEDI->GetXaxis()->SetTitle("ToF - Tpkup + #Delta (ns)");
	HSout_DEDI->GetYaxis()->SetTitle("Entries / N protons");
	HSout_DEDI->SetTitle("ToF dedicated bunches");
	TH1D *HSout_PARA = new TH1D("ToF Sout - parasitic bunches", "", n_vec_tof, xbins_tof);
	HSout_PARA->GetXaxis()->SetTitle("ToF - Tpkup + #Delta (ns)");
	HSout_PARA->GetYaxis()->SetTitle("Entries / N protons");
	HSout_PARA->SetTitle("ToF parasitic bunches");


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ANALYSIS ON SIN TOTAL SET

	int n_runs_SiTOT = SIN.size(); // length of the total Sin array
	int Run_SiTOT = 0;			   // variable to store the number of the current run in the loop
	Char_t R_iTOT[100];			   // variable to store the whole path of the current run

	TTree *t_iTOT, *t_pkup_iTOT; // ROOT trees for the 2 detectors of interest

	// variables to store the Pulse Intensity of each detector and bunch type
	float PI_Sin_DET1_dedi = 0;
	float PI_Sin_DET1_para = 0;
	float PI_Sin_DET2_dedi = 0;
	float PI_Sin_DET2_para = 0;
	float PI_Sin_DET3_dedi = 0;
	float PI_Sin_DET3_para = 0;
	float PI_Sin_DET4_dedi = 0;
	float PI_Sin_DET4_para = 0;
	float PI_Sin_DET7_dedi = 0;
	float PI_Sin_DET7_para = 0;
	float PI_Sin_DET8_dedi = 0;
	float PI_Sin_DET8_para = 0;

	cout << "------------------------------------------" << endl;
	cout << "SORTING Sin total RUNS" << endl;
	cout << "------------------------------------------" << endl;

	// loop over all the runs of Sin
	for (int k = 0; k < n_runs_SiTOT; ++k)
	{
		Run_SiTOT = SIN[k];
		sprintf(R_iTOT, "%s%i%s", prefix.c_str(), Run_SiTOT, suffix.c_str());
		cout << "Now Sorting " << R_iTOT << endl;

		// check to see if the current run is included in the set of runs of the various detectors
		bool check_det1 = Sin_DET1_SET.count(Run_SiTOT) > 0;
		bool check_det2 = Sin_DET2_SET.count(Run_SiTOT) > 0;
		bool check_det3 = Sin_DET3_SET.count(Run_SiTOT) > 0;
		bool check_det4 = Sin_DET4_SET.count(Run_SiTOT) > 0;
		bool check_det7 = Sin_DET7_SET.count(Run_SiTOT) > 0;
		bool check_det8 = Sin_DET8_SET.count(Run_SiTOT) > 0;

		cout << "DET 1:  " << check_det1 << endl;
		cout << "DET 2:  " << check_det2 << endl;
		cout << "DET 3:  " << check_det3 << endl;
		cout << "DET 4:  " << check_det4 << endl;
		cout << "DET 7:  " << check_det7 << endl;
		cout << "DET 8:  " << check_det8 << endl;

		// open the ROOT file corresponding to the current run
		TFile *f_i = TFile::Open(R_iTOT, "READ");

		if (!f_i || f_i->IsZombie() || !f_i->IsOpen())
		{
			std::cerr << "ERROR: cannot open file: " << R_iTOT << "\n";
			if (f_i)
			{
				delete f_i;
				f_i = nullptr;
			}
			continue; // skip this run
		}

		// read PKUP tree and extract the necessary leaves
		t_pkup_iTOT = (TTree *)f_i->Get("PKUP");

		// set read only branches
		t_pkup_iTOT->SetBranchStatus("*", 0);
		t_pkup_iTOT->SetBranchStatus("tflash", 1);
		t_pkup_iTOT->SetBranchStatus("BunchNumber", 1);
		t_pkup_iTOT->SetBranchStatus("PulseIntensity", 1);
		t_pkup_iTOT->SetBranchStatus("PSpulse", 1);

		t_pkup_iTOT->SetBranchAddress("tflash", &tflash_p);
		t_pkup_iTOT->SetBranchAddress("BunchNumber", &BunchNumberPK);
		t_pkup_iTOT->SetBranchAddress("PulseIntensity", &PulseIntensityPK);
		t_pkup_iTOT->SetBranchAddress("PSpulse", &PSpulsePK);

		// extract total number of protons (pulse intensity) from PKUP

		Long64_t nentryPK = t_pkup_iTOT->GetEntriesFast(); // get the total entries of the tree
		if (nentryPK < 0)
			nentryPK = t_pkup_iTOT->GetEntries();

		// loop over all the entries
		for (Long64_t iP = 0; iP < nentryPK; ++iP)
		{

			t_pkup_iTOT->GetEntry(iP);

			// For each detector and bunch type, the Pulse Intensity is stored in the corresponding variable
			// DET1
			if ((check_det1 == true) && (PSpulsePK == 2))
			{
				PI_Sin_DET1_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det1 == true) && (PSpulsePK == 3))
			{
				PI_Sin_DET1_para += PulseIntensityPK;
			} // parasitic

			// DET2
			if ((check_det2 == true) && (PSpulsePK == 2))
			{
				PI_Sin_DET2_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det2 == true) && (PSpulsePK == 3))
			{
				PI_Sin_DET2_para += PulseIntensityPK;
			} // parasitic

			// DET3
			if ((check_det3 == true) && (PSpulsePK == 2))
			{
				PI_Sin_DET3_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det3 == true) && (PSpulsePK == 3))
			{
				PI_Sin_DET3_para += PulseIntensityPK;
			} // parasitic

			// DET4
			if ((check_det4 == true) && (PSpulsePK == 2))
			{
				PI_Sin_DET4_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det4 == true) && (PSpulsePK == 3))
			{
				PI_Sin_DET4_para += PulseIntensityPK;
			} // parasitic

			// DET7
			if ((check_det7 == true) && (PSpulsePK == 2))
			{
				PI_Sin_DET7_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det7 == true) && (PSpulsePK == 3))
			{
				PI_Sin_DET7_para += PulseIntensityPK;
			} // parasitic

			// DET8
			if ((check_det8 == true) && (PSpulsePK == 2))
			{
				PI_Sin_DET8_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det8 == true) && (PSpulsePK == 3))
			{
				PI_Sin_DET8_para += PulseIntensityPK;
			} // parasitic
		}

		// read FC-U tree and extract relevant leaves
		t_iTOT = (TTree *)f_i->Get("FC-U");

		t_iTOT->SetBranchStatus("*", 0);
    	t_iTOT->SetBranchStatus("detn", 1);
    	t_iTOT->SetBranchStatus("tof", 1);
    	t_iTOT->SetBranchStatus("amp", 1);
    	t_iTOT->SetBranchStatus("BunchNumber", 1);
    	t_iTOT->SetBranchStatus("PulseIntensity", 1);
    	t_iTOT->SetBranchStatus("PSpulse", 1);


		t_iTOT->SetBranchAddress("detn", &detn);
		t_iTOT->SetBranchAddress("tof", &tof);
		t_iTOT->SetBranchAddress("amp", &amp);
		t_iTOT->SetBranchAddress("BunchNumber", &BunchNumber);
		t_iTOT->SetBranchAddress("PulseIntensity", &PulseIntensity);
		t_iTOT->SetBranchAddress("PSpulse", &PSpulse);

		// this method allows to read the 2 trees in parallel
		t_iTOT->AddFriend(t_pkup_iTOT);
		t_pkup_iTOT->BuildIndex("BunchNumber");

		// extract the total number of entries of the tree
		Long64_t nentry = t_iTOT->GetEntriesFast();
		if (nentry < 0)
			nentry = t_iTOT->GetEntries();
		cout << "Number of Entries: " << nentry << endl;

		int counter = 0;

		// loop over the entries of the tree
		for (Long64_t i = 0; i < nentry; ++i)
		{

			t_iTOT->GetEntry(i);

			// loop over the bunches
			if (BunchNumberPK == BunchNumber)
			{


				counter += 1;

				// I filter for each detector, adding also amplitude and time cuts
				// DET1
				if ((check_det1 == true) && (detn == 1) && (amp > cut_a_1) && (tof - tflash_p >= 1000))
				{

					// filter dedicated bunches and fill the corresponding histogram
					if (PSpulse == 2)
					{
						HSin_1_dedi->Fill(tof - tflash_p + cal_1);
					}

					// filter parasitic bunches and fill the corresponding histogram
					else if (PSpulse == 3)
					{
						HSin_1_para->Fill(tof - tflash_p + cal_1);
					}
				}

				// DET2
				if ((check_det2 == true) && (detn == 2) && (amp > cut_a_2) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSin_2_dedi->Fill(tof - tflash_p + cal_2);
					}

					else if (PSpulse == 3)
					{
						HSin_2_para->Fill(tof - tflash_p + cal_2);
					}
				}

				// DET3
				if ((check_det3 == true) && (detn == 3) && (amp > cut_a_3) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSin_3_dedi->Fill(tof - tflash_p + cal_3);
					}

					else if (PSpulse == 3)
					{
						HSin_3_para->Fill(tof - tflash_p + cal_3);
					}
				}

				// DET4
				if ((check_det4 == true) && (detn == 4) && (amp > cut_a_4) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSin_4_dedi->Fill(tof - tflash_p + cal_4);
					}

					else if (PSpulse == 3)
					{
						HSin_4_para->Fill(tof - tflash_p + cal_4);
					}
				}

				// DET7
				if ((check_det7 == true) && (detn == 7) && (amp > cut_a_7) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSin_7_dedi->Fill(tof - tflash_p + cal_7);
					}

					else if (PSpulse == 3)
					{
						HSin_7_para->Fill(tof - tflash_p + cal_7);
					}
				}

				// DET8
				if ((check_det8 == true) && (detn == 8) && (amp > cut_a_8) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSin_8_dedi->Fill(tof - tflash_p + cal_8);
					}

					else if (PSpulse == 3)
					{
						HSin_8_para->Fill(tof - tflash_p + cal_8);
					}
				}
			}
		} // close loop on tree entries

		cout << "Processed entries: "<<counter << '\n';

	} // close loop on the runs

	// I normalize each histogram for the corresponding number of protons, by dividing by the Pulse Intensity
	HSin_1_dedi->Scale(1 / PI_Sin_DET1_dedi);
	HSin_1_para->Scale(1 / PI_Sin_DET1_para);
	HSin_2_dedi->Scale(1 / PI_Sin_DET2_dedi);
	HSin_2_para->Scale(1 / PI_Sin_DET2_para);
	HSin_3_dedi->Scale(1 / PI_Sin_DET3_dedi);
	HSin_3_para->Scale(1 / PI_Sin_DET3_para);
	HSin_4_dedi->Scale(1 / PI_Sin_DET4_dedi);
	HSin_4_para->Scale(1 / PI_Sin_DET4_para);
	HSin_7_dedi->Scale(1 / PI_Sin_DET7_dedi);
	HSin_7_para->Scale(1 / PI_Sin_DET7_para);
	HSin_8_dedi->Scale(1 / PI_Sin_DET8_dedi);
	HSin_8_para->Scale(1 / PI_Sin_DET8_para);

	// I combine the histograms corresponding to different detectors
	HSin_DEDI->Add(HSin_1_dedi, HSin_2_dedi);
	HSin_DEDI->Add(HSin_3_dedi, HSin_4_dedi);
	HSin_DEDI->Add(HSin_7_dedi, HSin_8_dedi);

	HSin_PARA->Add(HSin_1_para, HSin_2_para);
	HSin_PARA->Add(HSin_3_para, HSin_4_para);
	HSin_PARA->Add(HSin_7_para, HSin_8_para);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ANALYSIS ON SOUT TOTAL SET

	int n_runs_SoTOT = SOUT.size();
	int Run_SoTOT = 0;

	Char_t R_oTOT[100];
	TTree *t_oTOT, *t_pkup_oTOT;

	// variables to store the Pulse Intensity of each detector and bunch type
	float PI_Sout_DET1_dedi = 0;
	float PI_Sout_DET1_para = 0;
	float PI_Sout_DET2_dedi = 0;
	float PI_Sout_DET2_para = 0;
	float PI_Sout_DET3_dedi = 0;
	float PI_Sout_DET3_para = 0;
	float PI_Sout_DET4_dedi = 0;
	float PI_Sout_DET4_para = 0;
	float PI_Sout_DET7_dedi = 0;
	float PI_Sout_DET7_para = 0;
	float PI_Sout_DET8_dedi = 0;
	float PI_Sout_DET8_para = 0;

	cout << "------------------------------------------" << endl;
	cout << "SORTING Sout total RUNS" << endl;
	cout << "------------------------------------------" << endl;
	for (int k = 0; k < n_runs_SoTOT; ++k)
	{
		Run_SoTOT = SOUT[k];
		sprintf(R_oTOT, "%s%i%s", prefix.c_str(), Run_SoTOT, suffix.c_str());
		cout << "Now Sorting " << R_oTOT << endl;

		// check to see if the current run is included in the set of runs of the various detectors
		bool check_det1 = Sout_DET1_SET.count(Run_SoTOT) > 0;
		bool check_det2 = Sout_DET2_SET.count(Run_SoTOT) > 0;
		bool check_det3 = Sout_DET3_SET.count(Run_SoTOT) > 0;
		bool check_det4 = Sout_DET4_SET.count(Run_SoTOT) > 0;
		bool check_det7 = Sout_DET7_SET.count(Run_SoTOT) > 0;
		bool check_det8 = Sout_DET8_SET.count(Run_SoTOT) > 0;

		cout << "DET 1:  " << check_det1 << endl;
		cout << "DET 2:  " << check_det2 << endl;
		cout << "DET 3:  " << check_det3 << endl;
		cout << "DET 4:  " << check_det4 << endl;
		cout << "DET 7:  " << check_det7 << endl;
		cout << "DET 8:  " << check_det8 << endl;

		TFile *f_p_oTOT = TFile::Open(R_oTOT, "READ");

		if (!f_p_oTOT || f_p_oTOT->IsZombie() || !f_p_oTOT->IsOpen())
		{
			std::cerr << "ERROR: cannot open file: " << R_oTOT << "\n";
			if (f_p_oTOT)
			{
				delete f_p_oTOT;
				f_p_oTOT = nullptr;
			}
			continue; // skip this run
		}

		// read PKUP tree
		t_pkup_oTOT = (TTree *)f_p_oTOT->Get("PKUP");

		// set read only branches
    	t_pkup_oTOT->SetBranchStatus("*", 0);
    	t_pkup_oTOT->SetBranchStatus("tflash", 1);
    	t_pkup_oTOT->SetBranchStatus("BunchNumber", 1);
    	t_pkup_oTOT->SetBranchStatus("PulseIntensity", 1);
    	t_pkup_oTOT->SetBranchStatus("PSpulse", 1);

		t_pkup_oTOT->SetBranchAddress("tflash", &tflash_p);
		t_pkup_oTOT->SetBranchAddress("BunchNumber", &BunchNumberPK);
		t_pkup_oTOT->SetBranchAddress("PulseIntensity", &PulseIntensityPK);
		t_pkup_oTOT->SetBranchAddress("PSpulse", &PSpulsePK);

		// extract N protons from PKUP
		Long64_t nentryPK = t_pkup_oTOT->GetEntriesFast();
		if (nentryPK < 0)
			nentryPK = t_pkup_oTOT->GetEntries();

		for (Long64_t iP = 0; iP < nentryPK; ++iP)
		{
			t_pkup_oTOT->GetEntry(iP);

			// For each detector and bunch type, I store the Pulse Intensity in the corresponding variable
			// DET1
			if ((check_det1 == true) && (PSpulsePK == 2))
			{
				PI_Sout_DET1_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det1 == true) && (PSpulsePK == 3))
			{
				PI_Sout_DET1_para += PulseIntensityPK;
			} // parasitic

			// DET2
			if ((check_det2 == true) && (PSpulsePK == 2))
			{
				PI_Sout_DET2_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det2 == true) && (PSpulsePK == 3))
			{
				PI_Sout_DET2_para += PulseIntensityPK;
			} // parasitic

			// DET3
			if ((check_det3 == true) && (PSpulsePK == 2))
			{
				PI_Sout_DET3_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det3 == true) && (PSpulsePK == 3))
			{
				PI_Sout_DET3_para += PulseIntensityPK;
			} // parasitic

			// DET4
			if ((check_det4 == true) && (PSpulsePK == 2))
			{
				PI_Sout_DET4_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det4 == true) && (PSpulsePK == 3))
			{
				PI_Sout_DET4_para += PulseIntensityPK;
			} // parasitic

			// DET7
			if ((check_det7 == true) && (PSpulsePK == 2))
			{
				PI_Sout_DET7_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det7 == true) && (PSpulsePK == 3))
			{
				PI_Sout_DET7_para += PulseIntensityPK;
			} // parasitic

			// DET8
			if ((check_det8 == true) && (PSpulsePK == 2))
			{
				PI_Sout_DET8_dedi += PulseIntensityPK;
			} // dedicated
			else if ((check_det8 == true) && (PSpulsePK == 3))
			{
				PI_Sout_DET8_para += PulseIntensityPK;
			} // parasitic
		}

		TFile *f_oTOT = TFile::Open(R_oTOT, "READ");
		// read FC-U tree
		t_oTOT = (TTree *)f_oTOT->Get("FC-U");

        // set read only branches
    	t_oTOT->SetBranchStatus("*", 0);
    	t_oTOT->SetBranchStatus("detn", 1);
    	t_oTOT->SetBranchStatus("tof", 1);
    	t_oTOT->SetBranchStatus("amp", 1);
    	t_oTOT->SetBranchStatus("BunchNumber", 1);
    	t_oTOT->SetBranchStatus("PulseIntensity", 1);
    	t_oTOT->SetBranchStatus("PSpulse", 1);

		t_oTOT->SetBranchAddress("detn", &detn);
		t_oTOT->SetBranchAddress("tof", &tof);
		t_oTOT->SetBranchAddress("amp", &amp);
		t_oTOT->SetBranchAddress("BunchNumber", &BunchNumber);
		t_oTOT->SetBranchAddress("PulseIntensity", &PulseIntensity);
		t_oTOT->SetBranchAddress("PSpulse", &PSpulse);

		t_oTOT->AddFriend(t_pkup_oTOT);
		t_pkup_oTOT->BuildIndex("BunchNumber");

		Long64_t nentry = t_oTOT->GetEntriesFast();
		if (nentry < 0)
			nentry = t_oTOT->GetEntries();

		cout << "Number of Entries: " << nentry << endl;
		
		int counter=0;

		for (Long64_t i = 0; i < nentry; ++i)
		{
			t_oTOT->GetEntry(i);

			if (BunchNumberPK == BunchNumber)
			{
				counter +=1;

				// I filter for each detector, adding also amplitude and time cuts
				// DET1
				if ((check_det1 == true) && (detn == 1) && (amp > cut_a_1) && (tof - tflash_p >= 1000))
				{

					// filter dedicated bunches and fill the corresponding histogram
					if (PSpulse == 2)
					{
						HSout_1_dedi->Fill(tof - tflash_p + cal_1);
					}

					// filter parasitic bunches and fill the corresponding histogram
					else if (PSpulse == 3)
					{
						HSout_1_para->Fill(tof - tflash_p + cal_1);
					}
				}

				// DET2
				if ((check_det2 == true) && (detn == 2) && (amp > cut_a_2) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSout_2_dedi->Fill(tof - tflash_p + cal_2);
					}

					else if (PSpulse == 3)
					{
						HSout_2_para->Fill(tof - tflash_p + cal_2);
					}
				}

				// DET3
				if ((check_det3 == true) && (detn == 3) && (amp > cut_a_3) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSout_3_dedi->Fill(tof - tflash_p + cal_3);
					}

					else if (PSpulse == 3)
					{
						HSout_3_para->Fill(tof - tflash_p + cal_3);
					}
				}

				// DET4
				if ((check_det4 == true) && (detn == 4) && (amp > cut_a_4) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSout_4_dedi->Fill(tof - tflash_p + cal_4);
					}

					else if (PSpulse == 3)
					{
						HSout_4_para->Fill(tof - tflash_p + cal_4);
					}
				}

				// DET7
				if ((check_det7 == true) && (detn == 7) && (amp > cut_a_7) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSout_7_dedi->Fill(tof - tflash_p + cal_7);
					}

					else if (PSpulse == 3)
					{
						HSout_7_para->Fill(tof - tflash_p + cal_7);
					}
				}

				// DET8
				if ((check_det8 == true) && (detn == 8) && (amp > cut_a_8) && (tof - tflash_p >= 1000))
				{

					if (PSpulse == 2)
					{
						HSout_8_dedi->Fill(tof - tflash_p + cal_8);
					}

					else if (PSpulse == 3)
					{
						HSout_8_para->Fill(tof - tflash_p + cal_8);
					}
				}
			}
		} // close loop on tree entries

		cout << "Processed entries: "<<counter << '\n';

	} // close loop on the runs

	// Normalize for N protons
	HSout_1_dedi->Scale(1 / PI_Sout_DET1_dedi);
	HSout_1_para->Scale(1 / PI_Sout_DET1_para);
	HSout_2_dedi->Scale(1 / PI_Sout_DET2_dedi);
	HSout_2_para->Scale(1 / PI_Sout_DET2_para);
	HSout_3_dedi->Scale(1 / PI_Sout_DET3_dedi);
	HSout_3_para->Scale(1 / PI_Sout_DET3_para);
	HSout_4_dedi->Scale(1 / PI_Sout_DET4_dedi);
	HSout_4_para->Scale(1 / PI_Sout_DET4_para);
	HSout_7_dedi->Scale(1 / PI_Sout_DET7_dedi);
	HSout_7_para->Scale(1 / PI_Sout_DET7_para);
	HSout_8_dedi->Scale(1 / PI_Sout_DET8_dedi);
	HSout_8_para->Scale(1 / PI_Sout_DET8_para);

	// Add together histograms for different detectors
	HSout_DEDI->Add(HSout_1_dedi, HSout_2_dedi);
	HSout_DEDI->Add(HSout_3_dedi, HSout_4_dedi);
	HSout_DEDI->Add(HSout_7_dedi, HSout_8_dedi);

	HSout_PARA->Add(HSout_1_para, HSout_2_para);
	HSout_PARA->Add(HSout_3_para, HSout_4_para);
	HSout_PARA->Add(HSout_7_para, HSout_8_para);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// PLOTS

	// Tof histograms

	// dedicated bunches
	TCanvas *cTOF_dedi = new TCanvas("tof dedicated bunches", "tof dedicated bunches", 2000, 1500);
	cTOF_dedi->SetLogx();
	cTOF_dedi->SetLogy();
	HSin_DEDI->SetLineColor(kViolet + 1);
	int xmin_plot = 1e3;
	HSin_DEDI->GetXaxis()->SetRangeUser(xmin_plot, 1e8);
	HSin_DEDI->Draw("HIST");
	HSout_DEDI->SetLineColor(kCyan + 2);
	HSout_DEDI->GetXaxis()->SetRangeUser(xmin_plot, 1e8);
	HSout_DEDI->Draw("HIST SAME");
	TLegend *legTOF_dedi = new TLegend(0.15, 0.6, 0.35, 0.80);
	legTOF_dedi->AddEntry(HSin_DEDI, "Sin", "l");
	legTOF_dedi->AddEntry(HSout_DEDI, "Sout", "l");
	legTOF_dedi->SetTextSize(0.025);
	legTOF_dedi->Draw();
	cTOF_dedi->SaveAs("./OUTPUT/TOF_dedicated.png");

	// parasitic bunches
	TCanvas *cTOF_para = new TCanvas("tof parasitic bunches", "tof parasitic bunches", 2000, 1500);
	cTOF_para->SetLogx();
	cTOF_para->SetLogy();
	HSin_PARA->SetLineColor(kViolet + 1);
	HSin_PARA->GetXaxis()->SetRangeUser(xmin_plot, 1e8);
	HSin_PARA->Draw("HIST");
	HSout_PARA->SetLineColor(kCyan + 2);
	HSout_PARA->GetXaxis()->SetRangeUser(xmin_plot, 1e8);
	HSout_PARA->Draw("HIST SAME");
	TLegend *legTOF_para = new TLegend(0.15, 0.6, 0.35, 0.80);
	legTOF_para->AddEntry(HSin_PARA, "Sin", "l");
	legTOF_para->AddEntry(HSout_PARA, "Sout", "l");
	legTOF_para->SetTextSize(0.025);
	legTOF_para->Draw();
	cTOF_para->SaveAs("./OUTPUT/TOF_parasistic.png");

	// Transmission ratios
	// dedicated
	TH1D *HTransm_dedi = (TH1D *)HSin_DEDI->Clone("HTransm_dedi");
	HTransm_dedi->Divide(HSout_DEDI);
	HTransm_dedi->SetTitle("Transmission dedicated bunches");
	HTransm_dedi->SetLineColor(9);

	// parasitic
	TH1D *HTransm_para = (TH1D *)HSin_PARA->Clone("HTransm_para");
	HTransm_para->Divide(HSout_PARA);
	HTransm_para->SetTitle("Transmission parasitic bunches");
	HTransm_para->SetLineColor(6);

	// plot transmission ratios in the same canvas
	TCanvas *c_transm_tot = new TCanvas("c transm tot", "c transm tot", 2000, 1500);
	HTransm_dedi->Draw("HIST");
	HTransm_para->Draw("HIST SAME");
	c_transm_tot->SetLogx();
	TLegend *leg_transm = new TLegend(0.65, 0.15, 0.85, 0.35);
	leg_transm->AddEntry(HTransm_dedi, "Dedicated bunches", "l");
	leg_transm->AddEntry(HTransm_para, "Parasitic bunches", "l");
	leg_transm->SetTextSize(0.025);
	leg_transm->Draw();
	c_transm_tot->SaveAs("./OUTPUT/Transmission_dedipara.png");

	// Build ratio of the 2 transmissions to compare them numerically : transm dedi / transm para
	TH1D *H_ratio = (TH1D *)HTransm_dedi->Clone("H_ratio");
	H_ratio->Divide(HTransm_para);
	H_ratio->SetTitle("Ratio of transmissions (dedicated/parasitic bunches)");
	H_ratio->SetLineColor(kOrange - 7);
	int rebin = 2;
	H_ratio->Rebin(rebin);
	H_ratio->Scale(1. / rebin);
	H_ratio->GetXaxis()->SetRangeUser(1.66e3, 1e8);
	TCanvas *cRatio = new TCanvas("c Ratio", "c Ratio", 2000, 1500);
	TLine *line = new TLine(1.66e3, 1, 1e8, 1);
	H_ratio->Draw();
	line->Draw("SAME");
	cRatio->SetLogx();
	cRatio->SaveAs("./OUTPUT/Transm_ratio_dedipara.png");

	// Histogram to group the transmission ratios and quantify mean and sigma
	TH1F *H_distr = new TH1F("Transmission ratio distribution", "Transmission ratio distribution", 50, 0., 1.3);
	int bin_start = H_ratio->FindBin(1.66e3); // start to fill the histogram from 1660 ns because before that the ratio is zero
	for (int i = bin_start; i <= H_ratio->GetNbinsX(); ++i)
	{
		H_distr->Fill(H_ratio->GetBinContent(i));
	}
	TCanvas *c_distr = new TCanvas("CH", "Canvas H", 2000, 1500);
	H_distr->SetLineWidth(2);
	H_distr->GetYaxis()->SetTitle("Entries");
	H_distr->GetXaxis()->SetTitle("Transmission ratio");
	H_distr->Draw("HIST");
	c_distr->SaveAs("./OUTPUT/Transm_ratio_distribution_dedipara.png");

	//sum transmission of dedicated and parasitic bunches
	TH1D *HTransm_final = new TH1D("Total transmission (dedicated + parasitic)", "", n_vec_tof, xbins_tof);
	HTransm_final->SetTitle("Total transmission (dedicated + parasitic)");
	HTransm_final->GetXaxis()->SetTitle("ToF - Tpkup + #Delta (ns)");
	HTransm_final->GetYaxis()->SetTitle("Entries / N protons");
	HTransm_final->Add(HTransm_para, HTransm_dedi);
	HTransm_final->GetXaxis()->SetRangeUser(xmin_plot, 1e8);
	TCanvas *c_Transm_final= new TCanvas("C_Transm_final", "Canvas Transm final", 2000, 1500);
	HTransm_final->Draw("HIST");
	c_Transm_final->SetLogx();
	c_Transm_final->SaveAs("./OUTPUT/Transmission_total.png");




}
