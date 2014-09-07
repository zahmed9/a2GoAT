#include "GHistTaggerBinning.h"
#include "GTreeTagger.h"

Int_t   GHistTaggerBinning::TaggerBinningRangeMin = 0;
Int_t   GHistTaggerBinning::TaggerBinningRangeMax = -1;

void    GHistTaggerBinning::InitTaggerBinning(const Int_t min, const Int_t max)
{
    TaggerBinningRangeMin = min;
    TaggerBinningRangeMax = max;
}



GHistTaggerBinning::GHistTaggerBinning() :
    GHistScaCor(),
    bin()
{
    bin.SetOwner();
}

GHistTaggerBinning::GHistTaggerBinning(const char* name, const char* title, Int_t nbinsx, Double_t xlow, Double_t xup, Bool_t linkHistogram) :
    GHistScaCor(name, title, nbinsx, xlow, xup, linkHistogram),
    bin()
{
    bin.SetOwner();
}

GHistTaggerBinning::~GHistTaggerBinning()
{

}

void    GHistTaggerBinning::CreateBin()
{
    GHistScaCor*    hist = new GHistScaCor(TString(GetName()).Append("_bin").Append(TString::Itoa(bin.GetEntriesFast()+TaggerBinningRangeMin, 10)).Data(),
                                           TString(GetTitle()).Append(" bin").Append(TString::Itoa(bin.GetEntriesFast()+TaggerBinningRangeMin, 10)).Data(),
                                           GetNbinsX(),
                                           GetXmin(),
                                           GetXmax(),
                                           kFALSE);
    bin.Add(hist);
}

void    GHistTaggerBinning::ExpandBin(const Int_t newSize)
{
    while(bin.GetEntriesFast()<newSize)
        CreateBin();
}

Bool_t	GHistTaggerBinning::Add(const GHistTaggerBinning* h, Double_t c)
{
    GHistScaCor::Add(h, c);
    for(int i=0; i<h->bin.GetEntriesFast(); i++)
    {
        if(i>=bin.GetEntriesFast())
            CreateBin();
        ((GHistScaCor*)bin.At(i))->Add((GHistScaCor*)h->bin.At(i), c);
    }
}

void    GHistTaggerBinning::CalcResult()
{
    if(bin.GetEntriesFast()==0)
        return;

    GHistScaCor::Reset();
    for(int i=0; i<bin.GetEntriesFast(); i++)
        GHistScaCor::Add((GHistScaCor*)bin.At(i));
}

void    GHistTaggerBinning::Reset(Option_t* option)
{
    GHistScaCor::Reset(option);
    for(int i=0; i<bin.GetEntriesFast(); i++)
        ((GHistScaCor*)bin.At(i))->Reset(option);
    bin.Clear();
}

Int_t   GHistTaggerBinning::Fill(const Double_t value, const Int_t taggerChannel)
{
    if(taggerChannel==-1)
        return GHistScaCor::Fill(value);

    if(TaggerBinningRangeMax==-1)
    {
        if(taggerChannel>=bin.GetEntriesFast())
            ExpandBin(taggerChannel+1);
        return ((GHistScaCor*)bin.At(taggerChannel))->Fill(value);
    }
    else
    {
        if(taggerChannel<TaggerBinningRangeMin || taggerChannel>TaggerBinningRangeMax)
            return 0;
        if(taggerChannel>=(bin.GetEntriesFast()+TaggerBinningRangeMin))
            ExpandBin(taggerChannel+1-TaggerBinningRangeMin);
        return ((GHistScaCor*)bin.At(taggerChannel-TaggerBinningRangeMin))->Fill(value);
    }
}

Int_t   GHistTaggerBinning::Fill(const Double_t value, const GTreeTagger& tagger, const Bool_t CreateHistogramsForTaggerBinning)
{
    for(int i=0; i<tagger.GetNTagged(); i++)
    {
        if(CreateHistogramsForTaggerBinning)
            Fill(value, tagger.GetTagged_ch(i));
        else
            Fill(value);
    }
}

void	GHistTaggerBinning::Scale(Double_t c1, Option_t* option)
{
    GHistScaCor::Scale(c1, option);
    for(int i=0; i<bin.GetEntriesFast(); i++)
        ((GHistScaCor*)bin.At(i))->Scale(c1, option);
}

void    GHistTaggerBinning::ScalerReadCorrection(const Double_t CorrectionFactor, const Bool_t CreateHistogramsForSingleScalerReads)
{
    GHistScaCor::ScalerReadCorrection(CorrectionFactor, CreateHistogramsForSingleScalerReads);
    for(int i=0; i<bin.GetEntriesFast(); i++)
        ((GHistScaCor*)bin.At(i))->ScalerReadCorrection(CorrectionFactor, CreateHistogramsForSingleScalerReads);
}

void    GHistTaggerBinning::PrepareWriteList(GHistWriteList* arr, const char *name)
{
    if(!arr)
        return;

    if(bin.GetEntriesFast()==0)
        return  GHistScaCor::PrepareWriteList(arr, name);

    for(int i=0; i<bin.GetEntriesFast(); i++)
        GHistScaCor::Add((GHistScaCor*)bin.At(i));
    GHistScaCor::PrepareWriteList(arr, name);

    GHistWriteList* TaggerBinning    = arr->GetDirectory(TString("TaggerBinning"));
    for(int i=0; i<bin.GetEntriesFast(); i++)
    {
        GHistWriteList* BinDir  = TaggerBinning->GetDirectory(TString("Channel_").Append(TString::Itoa(i+TaggerBinningRangeMin, 10)));
        if(name)
            ((GHistScaCor*)bin.At(i))->PrepareWriteList(BinDir, TString(name).Append("_bin").Append(TString::Itoa(i+TaggerBinningRangeMin, 10)));
        else
            ((GHistScaCor*)bin.At(i))->PrepareWriteList(BinDir);
    }
}

Int_t   GHistTaggerBinning::Write(const char* name, Int_t option, Int_t bufsize)
{
    if(bin.GetEntriesFast()==0)
    {
        return  GHistScaCor::Write(name, option, bufsize);
    }

    TDirectory* parentDir   = gDirectory;
    TDirectory* dir         = GetCreateDirectory("TaggerBinning");

    TString nameBuffer;
    Int_t res   = 0;
    for(int i=0; i<bin.GetEntriesFast(); i++)
    {
        if(name)
            nameBuffer  = name;
        else
            nameBuffer  = GetName();
        nameBuffer.Append("_Ch");
        nameBuffer.Append(TString::Itoa(i+TaggerBinningRangeMin, 10));

        dir->cd();
        GetCreateDirectory(TString("Channel_").Append(TString::Itoa(i+TaggerBinningRangeMin, 10)).Data())->cd();

        GHistScaCor::Add((GHistScaCor*)bin.At(i));
        res += ((GHistScaCor*)bin.At(i))->Write(nameBuffer.Data(), option, bufsize);
    }

    parentDir->cd();
    if(name)
        nameBuffer  = name;
    else
        nameBuffer  = GetName();
    res += GHistScaCor::Write(nameBuffer.Data(), option, bufsize);
}

