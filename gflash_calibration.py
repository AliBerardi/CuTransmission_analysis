####################################################################################################################################################
# The goal of this code is to perform the calibration of the gamma-flash of each detector with the time registered by the PKUP.
# The first function, data(), takes as input the file gflash_calibration.cmnd and builds a list with the full data path.
# The main function then extracts two dataframes, one for PKUP and one for FC-U. For each it extracts all the gamma-flash values,
# by reading the corresponding branch, and then fills an histogram with the difference of these two values.
# In the FC-U case the 6 detectors are analysed separately, so that actually 6 histograms are built and plotted in the same canvas.
# On each, a gaussian fit is performed, and the mean and standard deviation are extracted.
####################################################################################################################################################


# import libraries
import ROOT
import numpy as np
import math

import configreader_cpp as cr

ROOT.EnableImplicitMT()


def data(cmnd_file_name: str):

    ROOT.gROOT.SetBatch(True)

    cfg = cr.ConfigReader(cmnd_file_name)

    prefix = cfg.get_string("prefix")
    suffix = cfg.get_string("suffix")

    runlist = cfg.get_int_vector("runlist")

    file_list = [prefix + str(run) + suffix for run in runlist]
    print("For runs: \n", runlist)

    return (file_list)


def main(cmnd_file_name: str):

    file_list = data(cmnd_file_name)

    cfg = cr.ConfigReader(cmnd_file_name)
    detlist = cfg.get_int_vector("detlist")

    print("--------------------------------")
    print("Building gamma-flash calibration")
    print("--------------------------------")

    # define the dataframes for the 2 detectors
    df_pk = ROOT.RDataFrame("PKUP", file_list)
    df_fc = ROOT.RDataFrame("FC-U", file_list)

    # extract Tflash from PKUP
    d_tg_pkup = df_pk.AsNumpy(["tflash"])
    tg_pkup = d_tg_pkup["tflash"]
    TPKUP = np.array(list(set(tg_pkup)))

    # Dictionary to hold filtered dataframes (contains one df for each detctor)
    df_FCU = {}
    d_tg_FCU = {}
    tg_FCU = {}  # dictionary to store gamma flashes of the different detectors

    means = {}
    sigmas = {}
    Entries = {}

    means_err = {}
    sigmas_err = {}

    HistoDiff = []

    for i in detlist:

        CUT_det = f'detn=={i}'  # define detector choice

        df_FCU[i] = df_fc.Filter(CUT_det)  # define dataframe for each detector

        # extract tflash
        d_tg_FCU[i] = df_FCU[i].AsNumpy(["tflash"])
        tg_FCU[i] = d_tg_FCU[i]["tflash"]

        TFlash = np.array(list(set(tg_FCU[i])))
        Difference = np.array([a - b for a, b in zip(TPKUP, TFlash)])

        histo = ROOT.TH1F(
            f"difference with detector {i}", f"PKUP signal - tgflash of FCU{i}", 250, 450, 850)
        for x in Difference:
            histo.Fill(x)

        histo.GetXaxis().SetTitle("T (ns)")
        histo.GetYaxis().SetTitle("Entries")

        ROOT.gStyle.SetOptStat(0)  # disable default stat box

        HistoDiff.append(histo)

        Results = histo.Fit('gaus', 'S', '', 615, 715)

        amplitude = Results.Parameter(0)
        means[i] = Results.Parameter(1)
        sigmas[i] = Results.Parameter(2)
        Entries[i] = histo.GetEntries()
        means_err[i] = Results.Error(1)
        sigmas_err[i] = Results.Error(2)

    pavetexts = []

    c = ROOT.TCanvas('calib', 'calib', 2400, 1350)

    c.Divide(2, 3)

    for h in range(0, 6):

        c.cd(h+1)
        HistoDiff[h].Draw()

        if h < 4:
            a = h+1
        elif h >= 4:
            a = h+3

        legend = ROOT.TPaveText(0.75, 0.45, 0.95, 0.88, "NDC")
        legend.SetBorderSize(1)
        legend.SetFillColor(0)
        legend.SetTextAlign(12)
        legend.SetTextSize(0.04)
        legend.AddText(f"Entries = "+str(Entries[a]))
        legend.AddText(
            f"Mean = " + str(round(means[a], 3)) + " +/- " + str(round(means_err[a], 3)) + " ns ")
        legend.AddText(
            f"Sigma = " + str(round(sigmas[a], 3)) + " +/- " + str(round(sigmas_err[a], 3)) + " ns")
        legend.Draw("same")
        pavetexts.append(legend)

    c.Draw()

    c.SaveAs("./OUTPUT/gflashCAL.png")


if __name__ == "__main__":
    main("./input_files/gflash_calibration.cmnd")
