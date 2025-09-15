########################################################################################################################################################################
#  This program analyses single runs and for each creates the amplitude spectra, normalized to the number of protons.
#  It is organized in functions.
#  The first function in this program builds the data path reading input from a cmnd file.
#  The next function builds the dataframe for the detector of interest.
#  The following function reads the PKUP detector tree and calculates the pulse intensity of each run, which will be used to normalize the histograms.
#  The next function builds the amplitude histograms with cuts. It returns a tuple with the list of all the amplitude histograms
#  (one for each run), the list of the position of the bin with the maximum amplitude, the number of entries and the pulse intensity of each run.
#  There is a last function which can be used to plot the full amplitude histogram of a few runs. For example if, from the general plot, a strange behaviour is observed,
#  this can be useful to analyze in detail the problematic runs.
########################################################################################################################################################################

# import libraries
import ROOT
import numpy as np
import pandas as pd
import math
from array import array
from datetime import datetime
import sys
import os
import ast

import configreader_cpp as cr
ROOT.ROOT.EnableImplicitMT()


def data(DET: int, run_type: str, cmnd_name: str):

    cfg = cr.ConfigReader(cmnd_name)
    Sin = cfg.get_int_vector("Sin")
    Sout = cfg.get_int_vector("Sout")

    if DET == 1:
        Sin = cfg.get_int_vector("Sin_DET1")
    if DET == 8:
        Sin = cfg.get_int_vector("Sin_DET8")

    if run_type == "Sin":
        runlist = Sin
    if run_type == "Sout":
        runlist = Sout

    prefix = cfg.get_string("prefix")
    suffix = cfg.get_string("suffix")

    filelist = [prefix + str(run) + suffix for run in runlist]

    return (filelist, runlist)


# Build dataframe for the detector of interest
def dataframe(DET: int, run_type: str, cmnd_name: str):

    filelist = data(DET, run_type, cmnd_name)[0]

    # I create a list which I fill with dataframes corresponding to my detector (FC-U) for each run
    DF_det = []
    for file in filelist:
        df = ROOT.RDataFrame("FC-U", file)
        # filter dataframe for the dector of interest
        df = df.Filter(f'detn=={DET}')
        DF_det.append(df)
    return (DF_det)


# extract Pulse Intensity from PKUP
def PulseInt(DET: int, run_type: str, cmnd_name: str):

    filelist = data(DET, run_type, cmnd_name)[0]

    DF_PK_S = []
    for file in filelist:
        df_p = ROOT.RDataFrame("PKUP", file)
        DF_PK_S.append(df_p)

    d_PI = {}
    PI = {}
    PI_TOT = {}
    for k in range(0, len(DF_PK_S)):
        arr = DF_PK_S[k].AsNumpy(["PulseIntensity"])["PulseIntensity"]
        PI_TOT[k] = float(np.sum(arr, dtype=np.float64))
    return (PI_TOT)


# Create the amplitude histograms
def Histograms(DET: int, run_type: str, cmnd_name: str, nbins: int):

    runlist = data(DET, run_type, cmnd_name)[1]
    DF_det = dataframe(DET, run_type, cmnd_name)
    PI_TOT = PulseInt(DET, run_type, cmnd_name)

    cfg = cr.ConfigReader(cmnd_name)

    if DET == 1:
        cut_a = cfg.get_double("cut_a_det1")
    if DET == 2:
        cut_a = cfg.get_double("cut_a_det2")
    if DET == 3:
        cut_a = cfg.get_double("cut_a_det3")
    if DET == 4:
        cut_a = cfg.get_double("cut_a_det4")
    if DET == 7:
        cut_a = cfg.get_double("cut_a_det7")
    if DET == 8:
        cut_a = cfg.get_double("cut_a_det8")

    print("Building amplitude histograms")
    print("For DETECTOR: ", DET)
    print("With amplitude threshold: ", int(cut_a))
    print(f"Processing {run_type} runs: \n", runlist)

    # define dictionaries to store the amplitude values
    d_Amp = {}
    Amp = {}
    # define lists to store the histograms, the number of entries and the x position of the maximum bin

    Histos_CUTamp = []
    Entries = []
    MAX_BIN = []

    for i in range(0, len(DF_det)):  # loop over the datframes for the different runs

        # For each run, I extract all the amplitudes from the corresponding dataframe and put them in a dictionary
        d_Amp[i] = DF_det[i].AsNumpy(["amp"])
        Amp[i] = d_Amp[i]["amp"]
        Amplitude = np.array(Amp[i])  # convert the list into a numpy array

        # histogram with cut on amplitude
        Amplitude_cut = Amplitude[Amplitude > cut_a]
        histo_cut = ROOT.TH1F(
            f"Amplitude_histo_cut_{i}", f"Amplitudes detector {DET} with cut - {run_type}", nbins, 0, 45.e+3)
        for j in Amplitude_cut:
            histo_cut.Fill(j)

        histo_cut.GetXaxis().SetTitle("Amplitude (channels)")
        histo_cut.GetYaxis().SetTitle("Entries / N protons")
        Entries.append(int(Amplitude_cut.size))

        histo_cut.Scale(1/PI_TOT[i])

        Histos_CUTamp.append(histo_cut)

        # Get the position of the maximum bin and store it in a list
        max_bin = histo_cut.GetMaximumBin()
        x_max = histo_cut.GetBinCenter(max_bin)
        MAX_BIN.append(x_max)

    return (Histos_CUTamp, MAX_BIN, Entries, PI_TOT)


# You can call this function if you want to draw the amplitudes of some runs.

def Plot(DET: int, run_type: str, cmnd_name: str, nbins: int, first_run_plot: int, last_run_plot: int):

    runlist = data(DET, run_type, cmnd_name)[1]
    DF_det = dataframe(DET, run_type, cmnd_name)
    PI_TOT = PulseInt(DET, run_type, cmnd_name)

    print("---------------------------------------------------------------------")
    print(
        f"Plotting full amplitude histograms of runs from {first_run_plot} to {last_run_plot-1}")

    d_Amp = {}
    Amp = {}
    Histos = []

    for i in range(0, len(DF_det)):
        # For each run, extract all the amplitudes from the corresponding dataframe and put them in a dictionary
        d_Amp[i] = DF_det[i].AsNumpy(["amp"])
        Amp[i] = d_Amp[i]["amp"]
        Amplitude = np.array(Amp[i])  # convert the list into a numpy array

        # Define an histogram and fill it with the amplitudes
        histo = ROOT.TH1F(
            f"Amplitude_histo_{i}", f"Amplitudes detector {DET} - {run_type}", nbins, 0, 45.e+3)
        for x in Amplitude:
            histo.Fill(x)

        histo.GetXaxis().SetTitle("Amplitude (channels)")
        histo.GetYaxis().SetTitle("Entries / N protons")

        histo.Scale(1/PI_TOT[i])  # divide by proton number

        Histos.append(histo)

    # Draw histogram of amplitudes

    colors = [ROOT.kBlue, 4, 6, 7, 8, 9, 28, 30, 32, 34, 36, 38, 40, 41, 42, 44, 46, 48, 50, 52,
              55, 57, 58, 60, 62, 64, 65, 67, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90]  # 2
    legend = ROOT.TLegend(0.6, 0.38, 0.85, 0.85)
    c_CUT = ROOT.TCanvas('Amplitudes', 'stability amp CUTS', 1200, 750)
    ROOT.gStyle.SetOptStat(0)
    for hc, i in zip(range(first_run_plot, last_run_plot), range(0, (last_run_plot-first_run_plot))):
        Histos[hc].SetLineColor(colors[i])
        Histos[hc].Draw("HIST same")
        legend.AddEntry(Histos[hc], f"run 121{runlist[hc]} : {hc}", "l")
    legend.Draw()
    c_CUT.SaveAs(
        f"./OUTPUT/Amplitudes_det{DET}_{run_type}_from{first_run_plot}_to{last_run_plot-1}.png")


if __name__ == "__main__":
    # Histograms(DET=3, run_type="Sin", cmnd_name="./input_files/Histograms_lessRunsforSC.cmnd", nbins=150)
    Plot(DET=3, run_type="Sin", cmnd_name="./input_files/Histograms_AllRuns.cmnd",
         nbins=150, first_run_plot=5, last_run_plot=11)
