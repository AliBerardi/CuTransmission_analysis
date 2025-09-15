####################################################################################################################################################
# This code imports 14 runlists from Efficiency_plot_runlists.cmnd.
# For each of these runlists, 2 ampitude spectra are plotted: one over the full range and one with only the amplitudes above a threshold (cut_a).
# Both are normalized to the number of incident protons, that is extracted from the detector PKUP by reading the tree 'Pulse Intensity'.
# Then, the efficiency is computed as the integral under the curve of the histogram with the amplitude threshold.
# A graph of the efficiency as a function of the runlist number is built.
#
# By passing their value when calling the main function, it is possible to select the detector for which the analysis needs to be performed.
####################################################################################################################################################


# import libraries
import ROOT
import numpy as np
import pandas as pd
import math

import configreader_cpp as cr


# Speedups + headless plotting
ROOT.ROOT.EnableImplicitMT()

# function to read the data


def data(cmnd_file_name: str):

    # I import the 14 groups of runs
    cfg = cr.ConfigReader(cmnd_file_name)
    runlist1 = cfg.get_int_vector("runlist1")
    runlist2 = cfg.get_int_vector("runlist2")
    runlist3 = cfg.get_int_vector("runlist3")
    runlist4 = cfg.get_int_vector("runlist4")
    runlist5 = cfg.get_int_vector("runlist5")
    runlist6 = cfg.get_int_vector("runlist6")
    runlist7 = cfg.get_int_vector("runlist7")
    runlist8 = cfg.get_int_vector("runlist8")
    runlist9 = cfg.get_int_vector("runlist9")
    runlist10 = cfg.get_int_vector("runlist10")
    runlist11 = cfg.get_int_vector("runlist11")
    runlist12 = cfg.get_int_vector("runlist12")
    runlist13 = cfg.get_int_vector("runlist13")
    runlist14 = cfg.get_int_vector("runlist14")

    groups = [runlist1, runlist2, runlist3, runlist4, runlist5, runlist6, runlist7,
              runlist8, runlist9, runlist10, runlist11, runlist12, runlist13, runlist14]

    # Path to the files
    prefix = cfg.get_string("prefix")
    suffix = cfg.get_string("suffix")

    file_list = []

    for runs in groups:
        v = ROOT.std.vector('string')()
        for r in runs:
            v.push_back(f"{prefix}{r}{suffix}")
        file_list.append(v)

    return file_list


# function to extract the dataframe from FC-U, filtered for the selected detector
def FC_U(DET: int, cmnd_file_name: str):

    file_list = data(cmnd_file_name)

    # I define the dataframes, reading the tree FC-U and filtering for the detector that I chose at the beginning
    DF_det = []

    for v in file_list:
        df = ROOT.RDataFrame("FC-U", v).Filter(f"detn=={DET}")
        DF_det.append(df)

    return (DF_det)


# This function can be called to check what is inside the dataframes
def preview_DF_det(DF_det: int, n=5):
    """
    Preview the first n rows of each RDataFrame in a list.
    """
    for i, df in enumerate(DF_det):
        print(f"\n=== Runlist {i+1} (first {n} rows) ===")
        # Convert to numpy dict and then DataFrame
        data_dict = df.AsNumpy()
        df_pd = pd.DataFrame(data_dict)
        print(df_pd.head(n))


# Function to extract Pulse Intensity from detector PKUP
def PulseInt(cmnd_file_name: str):

    File_list = data(cmnd_file_name)

    # I define dataframes for each filelist, containing the tree corresponding to the PKUP detector, and group them in a list
    DF_PK = []

    for p in File_list:
        df_p = ROOT.RDataFrame("PKUP", p)
        DF_PK.append(df_p)

    # I define dictionaries to store the Pulse Intensity
    d_PI = {}
    PI = {}
    PI_TOT = {}

    for k in range(0, len(DF_PK)):
        d_PI[k] = DF_PK[k].AsNumpy(["PulseIntensity"])
        PI[k] = d_PI[k]["PulseIntensity"]

        # for each file_list, I want the total Pulse Intensity , so I sum all the PI values for that file and obtain the total PI
        PI_tot = 0
        for pi in range(0, len(PI[k])):
            PI_tot += PI[k][pi]
        PI_TOT[k] = PI_tot

    return (PI_TOT)


# main function
def main(DET: int, cmnd_file_name: str):

    DF_det = FC_U(DET, cmnd_file_name)
    PI_TOT = PulseInt(cmnd_file_name)

    LEN = len(DF_det)

    # Amplitude cuts
    cfg = cr.ConfigReader(cmnd_file_name)
    cut_a_det1 = cfg.get_double("cut_a_det1")
    cut_a_det2 = cfg.get_double("cut_a_det2")
    cut_a_det3 = cfg.get_double("cut_a_det3")
    cut_a_det4 = cfg.get_double("cut_a_det4")
    cut_a_det7 = cfg.get_double("cut_a_det7")
    cut_a_det8 = cfg.get_double("cut_a_det8")

   # for each detector the corresponding amplitude threshold is selected
    if DET == 1:
        cut_a = cut_a_det1
    elif DET == 2:
        cut_a = cut_a_det2
    elif DET == 3:
        cut_a = cut_a_det3
    elif DET == 4:
        cut_a = cut_a_det4
    elif DET == 7:
        cut_a = cut_a_det7
    elif DET == 8:
        cut_a = cut_a_det8

    print(f"Analysing detector {DET}")
    print(f"With amplitude threshold: {int(cut_a)} channels")

    # define the dictionaries to store the amplitude values
    d_Amp = {}
    Amp = {}

    # define lists to store the histograms
    Histos = []
    Histos_CUTamp = []

    # Define list to store entries
    Entries = []

    # loop over the 14 filelists and for each build 2 amplitude histograms, one with and one without the cut
    for i in range(0, LEN):

        # extract the amplitude values from the dataframe and store them in an array
        d_Amp[i] = DF_det[i].AsNumpy(["amp"])
        Amp[i] = d_Amp[i]["amp"]
        Amplitude = np.array(Amp[i])

        # Fill the first histogram with the amplitudes
        histo = ROOT.TH1F(
            f"Amplitude_histo_{i}", f"Amplitudes detector {DET} ", 300, 0, 45.e+3)
        for x in Amplitude:
            histo.Fill(x)
        # set axis label and range
        histo.GetXaxis().SetTitle("Amplitude (channels)")
        histo.GetYaxis().SetTitle("Entries / N protons")
        histo.GetYaxis().SetRangeUser(0, 14.e-15)
        histo.Scale(1/PI_TOT[i])  # divide by proton number
        Histos.append(histo)  # add the histogram to the list

        # histogram with cut on amplitude
        Amplitude_cut = Amplitude[Amplitude > cut_a]
        histo_cut = ROOT.TH1F(
            f"Amplitude_histo_cut_{i}", f"Amplitudes detector {DET} with cut", 300, 0, 45.e+3)
        for j in Amplitude_cut:
            histo_cut.Fill(j)
        # set axis label and range
        histo_cut.GetYaxis().SetRangeUser(0, 14.e-15)
        histo_cut.GetXaxis().SetTitle("Amplitude (channels)")
        histo_cut.GetYaxis().SetTitle("Entries / N protons")
        # get entries number and store it in the list
        entries = histo_cut.GetEntries()
        Entries.append(entries)
        # divide by proton number
        histo_cut.Scale(1/PI_TOT[i])
        # add the histogram to the list
        Histos_CUTamp.append(histo_cut)

    #############################################################
    # compute efficiency (integral over amplitude threshold)

    Efficiency = []
    for j in range(0, LEN):
        # find the bin corresponding to the amplitude threshold
        bin_cut = Histos[j].FindBin(cut_a)
        bin_end = Histos[j].FindBin(45e+3)  # find the last bin
        eff = Histos[j].Integral(bin_cut, bin_end)
        Efficiency.append(eff)

    # Calculate error on efficiency
    Y_ERR = []
    for i in range(0, LEN):
        y_err = math.sqrt(Entries[i])/PI_TOT[i]
        Y_ERR.append(y_err)

    # build a graph of the effixiency as a function of the runlist number
    x_runs = np.arange(1, 15, dtype='float64')
    y_ef = np.array(Efficiency, dtype='float64')
    ex = np.zeros_like(x_runs, dtype='float64')  # No x errors
    ey = np.array(Y_ERR, dtype='float64')

    graph_ratio = ROOT.TGraphErrors(14, x_runs, y_ef, ex, ey)
    graph_ratio.SetTitle(
        f"Efficiency - detector {DET};Runlist;Efficiency (Counts / N protons)")
    graph_ratio.SetMarkerStyle(20)
    graph_ratio.SetMarkerSize(1.0)

    #############################################################

    # Drawing
    colors = [5, 28, 38, 7, 30, 3, 8, 6, 4, 9, 46, 91, 95, 1]

    # Amplitude histogram with full range
    c = ROOT.TCanvas('Amplitudes ', 'stability', 1920, 2080)
    legend = ROOT.TLegend(0.6, 0.45, 0.80, 0.85)
    legend.AddEntry(Histos[0], f"runlist 1 - Sin", "l")
    legend.AddEntry(Histos[1], f"runlist 2 - Sin", "l")
    legend.AddEntry(Histos[2], f"runlist 3 - Sin", "l")
    legend.AddEntry(Histos[3], f"runlist 4 - Sin", "l")
    legend.AddEntry(Histos[4], f"runlist 5 - Sout", "l")
    legend.AddEntry(Histos[5], f"runlist 6 - Sout", "l")
    legend.AddEntry(Histos[6], f"runlist 7 - Sout", "l")
    legend.AddEntry(Histos[7], f"runlist 8 - Sin", "l")
    legend.AddEntry(Histos[8], f"runlist 9 - Sin", "l")
    legend.AddEntry(Histos[9], f"runlist 10 - Sin", "l")
    legend.AddEntry(Histos[10], f"runlist 11 - Sin", "l")
    legend.AddEntry(Histos[11], f"runlist 12 - Sout", "l")
    legend.AddEntry(Histos[12], f"runlist 13 - Sout", "l")
    legend.AddEntry(Histos[13], f"runlist 14 - Sout", "l")

    c.cd()
    for h in range(LEN-1, -1, -1):
        Histos[h].SetStats(False)
        Histos[h].SetLineColor(colors[h])
        Histos[h].Draw("HIST same")
    legend.Draw()
    c.Update()
    c.SaveAs(f"./OUTPUT/Runlists_Amplitudes_det{DET}.png")

    # Amplitude histogram with cuts
    c_CUT = ROOT.TCanvas('Amplitudes w CUT', 'stability amp CUTS', 1920, 1080)
    legend_CUT = ROOT.TLegend(0.6, 0.45, 0.80, 0.85)
    legend_CUT.AddEntry(Histos_CUTamp[0], f"runlist 1 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[1], f"runlist 2 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[2], f"runlist 3 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[3], f"runlist 4 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[4], f"runlist 5 - Sout", "l")
    legend_CUT.AddEntry(Histos_CUTamp[5], f"runlist 6 - Sout", "l")
    legend_CUT.AddEntry(Histos_CUTamp[6], f"runlist 7 - Sout", "l")
    legend_CUT.AddEntry(Histos_CUTamp[7], f"runlist 8 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[8], f"runlist 9 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[9], f"runlist 10 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[10], f"runlist 11 - Sin", "l")
    legend_CUT.AddEntry(Histos_CUTamp[11], f"runlist 12 - Sout", "l")
    legend_CUT.AddEntry(Histos_CUTamp[12], f"runlist 13 - Sout", "l")
    legend_CUT.AddEntry(Histos_CUTamp[13], f"runlist 14 - Sout", "l")

    c_CUT.cd()
    for hc in range(LEN-1, -1, -1):
        Histos_CUTamp[hc].SetStats(False)
        Histos_CUTamp[hc].SetLineColor(colors[hc])
        Histos_CUTamp[hc].Draw("HIST same")
    legend_CUT.Draw()
    c_CUT.Update()
    c_CUT.SaveAs(f"./OUTPUT/Runlists_AmplitudesCUT_det{DET}.png")

    # draw efficiency graph
    c_ratio = ROOT.TCanvas("Ratio", "Canvas", 1920, 1080)
    c_ratio.cd()
    graph_ratio.Draw("APL")
    c_ratio.Update()
    c_ratio.SaveAs(f"./OUTPUT/Runlists_Efficiency_det{DET}.png")


if __name__ == "__main__":
    main(DET=2, cmnd_file_name="./input_files/Efficiency_plot_runlists.cmnd")
