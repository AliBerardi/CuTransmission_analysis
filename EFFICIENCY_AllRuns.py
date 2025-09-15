########################################################################################################################################################################################################################################
########################################################################################################################################################################################################################################
# This program imports the module Histograms from the file Histograms_AllRuns.py
# From the amplitude histograms imported it caluclates the efficiency.
# The efficiency is the number of counts above an amplitude threshold (cut_a), normalized to the number of protons.
# The efficiency values are plotted in a graph as a function of the run number, and then they are grouped in 2 histograms, separating the 2 sets of Sin or Sout, from which it is possible to extract the error associated to the efficiency.
# In the efficiency graph, for each set are plotted a horizontal line, corresponding to the mean value, and two dashed lines at a distance of 2 standard deviations from the mean.
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

    # Import data from Histograms module
    Histos_CUTamp, MAX_BIN, Entries, PI_TOT = Histograms(
        DET, run_type, cmnd_name, nbins)

    print("MAX_BIN", MAX_BIN)

    LEN = len(Histos_CUTamp)

    # Import data from cmnd file
    cfg = cr.ConfigReader(cmnd_name)
    Sin1 = cfg.get_int_vector("Sin1")
    Sin2 = cfg.get_int_vector("Sin2")
    Sout1 = cfg.get_int_vector("Sout1")
    Sout2 = cfg.get_int_vector("Sout2")

    if DET == 1:
        Sin2 = cfg.get_int_vector("Sin2_DET1")
    if DET == 8:
        Sin1 = cfg.get_int_vector("Sin1_DET8")

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

    print("---------------------------------------------------------------------")
    print("Building efficiency graph")

    #############################################################
    # Calculate efficiency
    Efficiency = []
    Y_ERR = []
    for j in range(0, LEN):
        bin_cut = Histos_CUTamp[j].FindBin(cut_a)
        bin_end = Histos_CUTamp[j].FindBin(45e+3)
        eff = Histos_CUTamp[j].Integral(bin_cut, bin_end)
        Efficiency.append(eff)

        y_err = math.sqrt(Entries[j])/PI_TOT[j]
        Y_ERR.append(y_err)

    x_runs = np.arange(0, LEN, dtype='float64')
    y_ratio = np.array(Efficiency, dtype='float64')
    ex = np.zeros_like(x_runs, dtype='float64')  # No x errors
    ey = np.array(Y_ERR, dtype='float64')

    # check for NaN/Inf
    finite = np.isfinite(y_ratio) & np.isfinite(ey)
    x_runs, y_ratio, ex, ey = x_runs[finite], y_ratio[finite], ex[finite], ey[finite]
    LEN = len(x_runs)

    # divide in 2 colors
    if run_type == "Sout":
        n = len(Sout1)
    if run_type == "Sin":
        n = len(Sin1)
    y_S1 = y_ratio[:n]
    y_S2 = y_ratio[n:]
    x_S1 = x_runs[:n]
    x_S2 = x_runs[n:]
    ey_S1 = ey[:n]
    ey_S2 = ey[n:]
    ex_S1 = ex[:n]
    ex_S2 = ex[n:]

    #############################################################
    # Histo to extract error on efficiency
    min = np.mean(y_ratio)-(0.5e-13)
    max = np.mean(y_ratio)+(0.5e-13)
    h_E1 = ROOT.TH1F("Efficiency histo 1", "Efficiency distribution detector " +
                     str(DET) + " - " + str(run_type)+"1", 50, min, max)
    h_E1.SetLineColor(ROOT.kBlue)
    for e in y_S1:
        h_E1.Fill(e)
    mean1 = h_E1.GetMean()
    sigma1 = h_E1.GetStdDev()
    h_E1.GetXaxis().SetTitle("Efficiency (Counts / N protons)")
    h_E1.GetYaxis().SetTitle("Entries")

    h_E2 = ROOT.TH1F("Efficiency histo 2", "Efficiency distribution detector " +
                     str(DET) + " - " + str(run_type)+"2", 50, min, max)
    h_E2.SetLineColor(ROOT.kGreen+2)
    for e2 in y_S2:
        h_E2.Fill(e2)
    mean2 = h_E2.GetMean()
    sigma2 = h_E2.GetStdDev()
    h_E2.GetXaxis().SetTitle("Efficiency (Counts / N protons)")
    h_E2.GetYaxis().SetTitle("Entries")

    # draw graph of efficiency
    graph1 = ROOT.TGraphErrors(n, np.array(x_S1, dtype='float64'), np.array(
        y_S1, dtype='float64'), np.array(ex_S1, dtype='float64'), np.array(ey_S1, dtype='float64'))
    graph1.SetTitle(
        f"Efficiency detector {DET} - {run_type}; Run; Efficiency (Counts / N protons)")
    graph1.SetMarkerColor(ROOT.kBlue)
    graph1.SetLineColor(ROOT.kBlue)
    graph1.SetMarkerStyle(20)
    n2 = len(x_S2)
    graph2 = ROOT.TGraphErrors(n2, np.array(x_S2, dtype='float64'), np.array(
        y_S2, dtype='float64'), np.array(ex_S2, dtype='float64'), np.array(ey_S2, dtype='float64'))
    graph2.SetMarkerColor(ROOT.kGreen+2)
    graph2.SetLineColor(ROOT.kGreen+2)
    graph2.SetMarkerStyle(20)
    GRAPH = ROOT.TMultiGraph()
    GRAPH.Add(graph1)
    GRAPH.Add(graph2)

    line1 = ROOT.TLine(0, mean1, n, mean1)
    line1.SetLineColor(ROOT.kRed)
    line1.SetLineWidth(2)
    line1u = ROOT.TLine(0, (mean1+(2*sigma1)), n, (mean1+(2*sigma1)))
    line1u.SetLineColor(ROOT.kRed)
    line1u.SetLineStyle(2)
    line1d = ROOT.TLine(0, (mean1-(2*sigma1)), n, (mean1-(2*sigma1)))
    line1d.SetLineColor(ROOT.kRed)
    line1d.SetLineStyle(2)

    line2 = ROOT.TLine(n, mean2, LEN, mean2)
    line2.SetLineColor(ROOT.kRed)
    line2.SetLineWidth(2)
    line2u = ROOT.TLine(n, (mean2+(2*sigma2)), LEN, (mean2+(2*sigma2)))
    line2u.SetLineColor(ROOT.kRed)
    line2u.SetLineStyle(2)
    line2d = ROOT.TLine(n, (mean2-(2*sigma2)), LEN, (mean2-(2*sigma2)))
    line2d.SetLineColor(ROOT.kRed)
    line2d.SetLineStyle(2)

    c_r2 = ROOT.TCanvas("Ratio", "Canvas", 1200, 750)
    c_r2.SetLeftMargin(0.15)
    GRAPH.Draw("APL")
    GRAPH.SetTitle(
        f"Efficiency detector {DET} - {run_type}; Run; Efficiency (Counts / N protons)")
    GRAPH.GetXaxis().SetLimits(0, LEN)
    line1.Draw("same")
    line1u.Draw("same")
    line1d.Draw("same")
    line2.Draw("same")
    line2u.Draw("same")
    line2d.Draw("same")
    c_r2.cd()
    pave = ROOT.TPaveText(0.74, 0.75, 0.96, 0.87, "NDC")
    pave.AddText(f"Threshold = {int(cut_a)} channels")
    pave.SetTextAlign(12)
    pave.SetTextSize(0.025)
    pave.SetTextFont(42)
    pave.SetBorderSize(1)
    pave.SetFillStyle(1001)
    pave.SetFillColor(ROOT.kWhite)
    pave.Draw()
    c_r2.Update()
    c_r2.SaveAs(f"./OUTPUT/Efficiency_singleruns_det{DET}_{run_type}.png")

    # Draw error graph
    c_E = ROOT.TCanvas("E", "Canvas", 1200, 750)
    c_E.Divide(2, 1)
    c_E.cd(1)
    h_E1.Draw("HIST")
    c_E.cd(2)
    h_E2.Draw("HIST")
    c_E.cd()
    pave = ROOT.TPaveText(0.70, 0.65, 0.99, 0.77, "NDC")
    pave.AddText(f"Threshold = {int(cut_a)} channels")
    pave.SetTextAlign(12)
    pave.SetTextSize(0.023)
    pave.SetTextFont(42)
    pave.SetBorderSize(1)
    pave.SetFillStyle(1001)
    pave.SetFillColor(ROOT.kWhite)
    pave.Draw()
    c_E.Update()
    c_E.SaveAs(
        f"./OUTPUT/EfficiencyHistogram_singleruns_det{DET}_{run_type}.png")


if __name__ == "__main__":
    main(DET=2, run_type="Sout",
         cmnd_name="./input_files/Histograms_lessRunsforSC.cmnd", nbins=300)
