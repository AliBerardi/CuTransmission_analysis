########################################################################################################################################################################################################################################
########################################################################################################################################################################################################################################
# This program imports the module Histograms from the file Histograms_AllRuns.py
# It takes the position of the maximum above the amplitude threshold  and plots it in a graph as a function of the run number.
# It makes a distinction between the 2 sets of data by plotting them in different colours.
# When calling the main function, it is possible to choose the detector (DET), select between Sample in and Sample out (run_type), choose the input file for which the
# analysis need  to be performed, and finally choose the number of bins of the amplitude histograms.
########################################################################################################################################################################################################################################
########################################################################################################################################################################################################################################


# import libraries
import ROOT
import numpy as np
import math

import configreader_cpp as cr
from Histograms_AllRuns import Histograms


def main(DET: int, run_type: str, cmnd_name: str, nbins: int):

    ROOT.gROOT.SetBatch(True)

    Histos_CUTamp, MAX_BIN, Entries, PI_TOT = Histograms(
        DET, run_type, cmnd_name, nbins)

    LEN = len(Histos_CUTamp)

    cfg = cr.ConfigReader(cmnd_name)
    Sin1 = cfg.get_int_vector("Sin1")
    Sin2 = cfg.get_int_vector("Sin2")
    Sout1 = cfg.get_int_vector("Sout1")
    Sout2 = cfg.get_int_vector("Sout2")

    if DET == 1:
        Sin2 = cfg.get_int_vector("Sin2_DET1")
    if DET == 8:
        Sin1 = cfg.get_int_vector("Sin1_DET8")

    print("---------------------------------------------------------------------")
    print("Building stability graph: position of maximum amplitude over the runs")

    # Build graph with maximum peak values

    x_runs = np.arange(LEN, dtype="float64")
    ex = np.zeros(LEN, dtype="float64")
    MAX_BIN_array = np.array(MAX_BIN, dtype='float64')
    bin_width = 45.e3/nbins
    y_err = bin_width/math.sqrt(12)
    Y_ERR = np.full(LEN, y_err)

    # separate set 1 and 2 because I want to plot them in different colours
    split_lengths = {
        "Sout": len(Sout1),
        "Sin": len(Sin1),
    }
    if run_type not in split_lengths:
        raise ValueError(
            f"Unknown run_type '{run_type}'. Expected 'Sin' or 'Sout'.")
    n = split_lengths[run_type]

    x_S1, x_S2 = x_runs[:n], x_runs[n:]
    y_S1, y_S2 = MAX_BIN_array[:n], MAX_BIN_array[n:]
    ex_S1, ex_S2 = ex[:n], ex[n:]
    ey_S1, ey_S2 = Y_ERR[:n], Y_ERR[n:]
    n2 = len(x_S2)

    graph1 = ROOT.TGraphErrors(n, np.array(x_S1, dtype='float64'), np.array(
        y_S1, dtype='float64'), np.array(ex_S1, dtype='float64'), np.array(ey_S1, dtype='float64'))
    graph1.SetMarkerColor(ROOT.kBlue)
    graph1.SetLineColor(ROOT.kBlue)
    graph1.SetMarkerStyle(20)
    graph2 = ROOT.TGraphErrors(n2, np.array(x_S2, dtype='float64'), np.array(
        y_S2, dtype='float64'), np.array(ex_S2, dtype='float64'), np.array(ey_S2, dtype='float64'))
    graph2.SetMarkerColor(ROOT.kGreen+2)
    graph2.SetLineColor(ROOT.kGreen+2)
    graph2.SetMarkerStyle(20)
    GRAPH = ROOT.TMultiGraph()
    GRAPH.Add(graph1)
    GRAPH.Add(graph2)

    ROOT.SetOwnership(graph1, False)
    ROOT.SetOwnership(graph2, False)
    ROOT.SetOwnership(GRAPH, False)

    c_r2 = ROOT.TCanvas("Maximum bin", "Canvas", 1200, 750)
    ROOT.SetOwnership(c_r2, False)
    GRAPH.Draw("A*L")
    GRAPH.SetTitle(
        f"Position of maximum amplitude detector {DET} - {run_type}; Run; Amplitude (channels)")
    GRAPH.GetXaxis().SetLimits(0, LEN)
    graph2.Draw("*L SAME")
    c_r2.Update()
    c_r2.SaveAs("./OUTPUT/max_bin.png")


if __name__ == "__main__":
    main(2, "Sin", "./input_files/Histograms_AllRuns.cmnd", 150)
