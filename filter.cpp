//Initialy for filtering out anomalies in daily yahoo data. Using database of 285k instruments.
//Data files probably were filtered on load for 20-50kb+ files(to conserve ram). 
//filtering = considering data point or period useless.
//Dev process was making 1 filtering function by 1 until no visually strange and too predictable edges were found in data. 
//After each wave of most predictable anomalies got filtered out, new anomalies were found and later manually confirmed using random selection and visualization tools.





bool File_Load_Class::checkIfMissingTooMuch_vAtsToEvenScan()
{
    il("CheckingDataAvailibility...")


    updateMaxAllowedMissingVAt_s();
    double minAvailibleRatioToAccept=1-r.set.ruMaxMissingDataRatio;

    int maxDataInRange=r.range1.end_pos_at-r.range1.start_pos_at;

    int someOrFullDataAvailibleCount=0;

    double potentialSomeOrFullDataAvailibleAvgRatio=0;


    int problemCount=0;


    //stage1 checks
    f0(v,dataVec.size()){
        if(dataVecStart[v] < r.range1.end_pos_at){

            someOrFullDataAvailibleCount++;

            int checkStartAt=max(dataVecStart[v],r.range1.start_pos_at);
            int checkEndAt=min(dataVecEnd(v),r.range1.end_pos_at);

            int dataInRangePotentialyAvailible = checkEndAt-checkStartAt;
            double potAvalibleRatio= (double)dataInRangePotentialyAvailible/maxDataInRange;
            potentialSomeOrFullDataAvailibleAvgRatio+=potAvalibleRatio;

        }
    }
    int missingRangesCompletely=dataVec.size()-someOrFullDataAvailibleCount;
    potentialSomeOrFullDataAvailibleAvgRatio/=(double)someOrFullDataAvailibleCount;

    if(missingRangesCompletely>=max_missing_vAts){
        il("missingRangesCompletely="+n((double)missingRangesCompletely/dataVec.size()));
        problemCount++;
    }
    double avalibleVAtRatio=(double)someOrFullDataAvailibleCount/dataVec.size();
    //double unavalibleToAvalibleVAtRatio=(double)missingRangesCompletely/dataVec.size();

    double potentialAvailibleRatio=(potentialSomeOrFullDataAvailibleAvgRatio*avalibleVAtRatio);
    if(potentialAvailibleRatio<minAvailibleRatioToAccept){
        il("potentialAvailibleRatio="+n(potentialAvailibleRatio));
        problemCount++;
    }
    if(problemCount>=1){
        il("MissingTooMuch:InStage1:SkippingScan")
        return 1;
    }


    //stage 2 checks
    double actualCloseDataAvailibleAvgRatio=0;
    double actualHiLoDataAvailibleAvgRatio=0;

    f0(v,dataVec.size()){
        if(dataVecStart[v] < r.range1.end_pos_at){

            int missingData=0;
            int closesAvailible=0;
            int hiLoAvailible=0;

            int checkStartAt=max(dataVecStart[v],r.range1.start_pos_at);
            int checkEndAt=min(dataVecEnd(v),r.range1.end_pos_at);

            f(atCheck,checkStartAt,checkEndAt){
               data_struct * dCheck = getData(v,atCheck);
              if(dataMissing(dCheck)){
                  missingData++;
              }else{
                  if(dCheck->closeOk)
                      closesAvailible++;
                  if(dCheck->hiLoOk)
                      hiLoAvailible++;
              }

            }
            actualCloseDataAvailibleAvgRatio+=(double)closesAvailible/(maxDataInRange-missingData);
            actualHiLoDataAvailibleAvgRatio+=(double)hiLoAvailible/(maxDataInRange-missingData);
        }
    }

    actualCloseDataAvailibleAvgRatio/=(double)someOrFullDataAvailibleCount;
    actualHiLoDataAvailibleAvgRatio/=(double)someOrFullDataAvailibleCount;

    double actualAvailibleHiLoRatio=(actualHiLoDataAvailibleAvgRatio*avalibleVAtRatio);
    if(actualAvailibleHiLoRatio<minAvailibleRatioToAccept){
        il("actualAvailibleHiLoRatio="+n(actualAvailibleHiLoRatio));
        problemCount++;
    }
    double actualAvailibleCloseRatio=(actualCloseDataAvailibleAvgRatio*avalibleVAtRatio);
    if(actualAvailibleCloseRatio<minAvailibleRatioToAccept){
        il("actualAvailibleCloseRatio="+n(actualAvailibleCloseRatio));
        problemCount++;
    }

    if(problemCount>=2){
        il("MissingTooMuch:SkippingScan")
        return 1;
    }else{
        il("OK");
        return 0;
    }


}




void File_Load_Class::filterData()
{
    if(!filterSet.enable)
        return;



    infoLine("Filtering...");

//NB takes quite long on on 50gb+ data, test for bottlenecks, maybe due to tempVec...
//NB step trough to check for bugs if time at some point


    vector<int>dataToRemove;
    vector<int>indexesToRemove;

    vector <int> dataVecEnd(dataVec.size());//only for filtering

    infoLine("seeking blank indexes");
    //% missing data index //could count data per index while loading data..... otherwise slows swap down alot NB NB
    //gather


        for(int o=0; o<timeVec.size(); o++){
            int missingCount=0;

            for(int i=0; i<dataVec.size(); i++){
              if(dataVec[i][o].CLOSE<=0)//nb could check more
                  missingCount++;
            }

            double badRatio=(double)missingCount/dataVec.size();

            if(badRatio>=filterSet.requiredBadRatioToSkipIndex){
                indexesToRemove.emplace_back(o);
            }
    }

        //rem
        int newSize=timeVec.size()-indexesToRemove.size();

        vector<vector<data_struct> > dataVecTemp;
        dataVecTemp.resize(dataVec.size());

    infoLine("pendingIndexRems..." + n(indexesToRemove.size()));

    for(int i = 0; i<dataVecTemp.size(); i++){
        int remVecAt=0;
        int nextToSkip;
        if(indexesToRemove.empty()){
            nextToSkip=-1;
        }else{
            nextToSkip=indexesToRemove[remVecAt];
        }

        dataVecTemp[i].resize(newSize);
    for(int o=0; o<timeVec.size(); o++){
        if(o==nextToSkip){
            remVecAt++;
            nextToSkip=indexesToRemove[remVecAt];
            continue;
        }
            dataVecTemp[i][o-remVecAt]=dataVec[i][o];
        }
    dataVec[i].clear();
    dataVec[i].shrink_to_fit();
    }
    dataVec.clear();

     infoLine("timevec index remove...");
     vector<qint64> timeVecTemp;
     timeVecTemp.resize(timeVec.size()-indexesToRemove.size());

         int remVecAt=0;
         int nextToSkip;
         if(indexesToRemove.empty()){
             nextToSkip=-1;
         }else{
             nextToSkip=indexesToRemove[remVecAt];
         }

     for(int o=0; o<timeVec.size(); o++){
         if(o==nextToSkip){
             remVecAt++;
             nextToSkip=indexesToRemove[remVecAt];
             continue;
         }
             timeVecTemp[o-remVecAt]=timeVec[o];
         }
     timeVec=timeVecTemp;
     timeVec.shrink_to_fit();

     timeVecTemp.clear();
     timeVecTemp.shrink_to_fit();

    infoLine("settingStartEndPositions...");

     fill(dataVecEnd.begin(),dataVecEnd.end(),0); //to avoid crash

     //get end pos
     for(int i=0; i<dataVecTemp.size(); i++){
         for(int o=dataVecTemp[i].size()-1; o>=0; o--){
             if(dataVecTemp[i][o].CLOSE>0 && dataVecTemp[i][o].LO!=numeric_limits<float>::max() && !isinf(dataVecTemp[i][o].CLOSE)){
                     dataVecEnd[i]=o;
                     break;
             }
         }
     }

     //get start
     for(int i=0; i<dataVecTemp.size(); i++){
         for(int o=0; o<dataVecEnd[i]; o++){
             if(dataVecTemp[i][o].CLOSE>0 && !isinf(dataVecTemp[i][o].CLOSE)){
                 dataVecStart[i]=o;
                 break;
             }
         }
     }




     if(dataVecTemp.empty()){
         infoLine("filter quit 1, all data filtered out");
         return;
      }

     dataProblems dataProb;


    infoLine("checkingProblems...");
    labelDataProblems(-1,-1,dataVecTemp,&dataVecEnd,dataProb);


  int lenTooShort=0;
  int problemRatioTooBig=0;
//gather too few or too faulty data
for(int i = 0; i<dataVecTemp.size(); i++){
    int len=dataVecEnd[i]-dataVecStart[i]+1;

    bool badResult=0;

    if(len>=filterSet.minDataLen){
        int okLen=0;

        for(int o=dataVecStart[i]; o<dataVecEnd[i]; ++o){
            if(dataVecTemp[i][o].hiLoOk){
                okLen++;
            }
        }
        if(okLen<filterSet.minDataLen){
            badResult=1;
            lenTooShort++;
        }else{
            int problems=len-okLen;
            double problemRatio=(double)problems/len;

            if(problemRatio>filterSet.maxProblemRatio){//nb adds settings
                badResult=1;
                problemRatioTooBig++;
            }
        }

    }else{
        badResult=1;
        lenTooShort++;
    }

    if(badResult){
        dataToRemove.emplace_back(i);
    }
}

if(lenTooShort!=0)
        infoLine("Data rem:LenTooShort "+n(lenTooShort));
if(problemRatioTooBig!=0)
        infoLine("Data rem:problemRatioTooBig "+n(problemRatioTooBig));





infoLine("resizeData");
//data move to mainVec and clean

int dataToRemoveAt=0;
int next_vAtToSkip;

if(dataToRemove.empty()){
   next_vAtToSkip=-1;
}else{
   next_vAtToSkip=dataToRemove[dataToRemoveAt];
}


dataVec.resize(dataVecTemp.size()-dataToRemove.size());

for(int i = 0; i<dataVecTemp.size(); i++){

    if(i == next_vAtToSkip){//skips data thats was to be removed

    dataToRemoveAt++;

    if(dataToRemoveAt!=dataToRemove.size())
    next_vAtToSkip=dataToRemove[dataToRemoveAt];

    continue;
    }

    int len=dataVecEnd[i]-dataVecStart[i]+1;
    dataVec[i-dataToRemoveAt].resize(len);

    copy(dataVecTemp[i].begin()+dataVecStart[i],dataVecTemp[i].begin()+dataVecEnd[i]+1,dataVec[i-dataToRemoveAt].begin());

    dataVecTemp[i].clear();//to reduce usage
    dataVecTemp[i].shrink_to_fit();
}

    dataVecTemp.clear();

for(int db=dataToRemove.size()-1; db>=0; db--){//set start end right and names
    removeExtraFileAt(dataToRemove[db],1);
}

    timeVec.shrink_to_fit();

    if(dataVec.empty()){
       infoLine("filter quit 2, all data filtered out");
       return;
    }


   printDataProblems(dataProb);

   infoLine("filter finished");
}

void File_Load_Class::labelDataProblemsAndPrint(int vAt, int at)
{
    if(loadingFiles)
        return;

    dataProblems dataProb;
    labelDataProblems(vAt,at,dataVec,NULL,dataProb);
    printDataProblems(dataProb);
}


void File_Load_Class::labelDataProblems(int vAtTarget, int at,vector<vector<data_struct>>&dataVecTemp,vector<int>*dataVecEnd_,dataProblems&dataProb)
{


    //bad prices test and ok assign


    int vAtFrom,vAtTo;
    if(vAtTarget==-1){
        vAtFrom=0;
        vAtTo=dataVecTemp.size();
    }else{
        vAtFrom=vAtTarget;
        vAtTo=vAtTarget+1;
    }


      for(int vAt=vAtFrom; vAt<vAtTo; vAt++){//nb coul;d make multi calss data propblems so close problems not too abd and can use in trades but not open etc
        if(dataVecTemp[vAt].empty())
            continue;



          int from,to;
          bool resetOk;
          if(at==-1){
              from=dataVecStart[vAt];

              if(dataVecEnd_!=NULL){
              to=(*dataVecEnd_)[vAt];//for temp data under filter
              resetOk=0;
              }else{
              to=dataVec[vAt].size()-1;//for later filtering
              resetOk=1;
              }
          }else{
              from=getDataIndex(vAt,at);
              if(from==-1)
                  continue;

              to=from;
              resetOk=1;
          }
          dataProb.totalData+=(to-from)+1;


          for(int o=from; o<=to; o++){

              if(resetOk){
                  dataVecTemp[vAt][o].hiLoOk=0;
                  dataVecTemp[vAt][o].closeOk=0;
              }
              data_struct oo=dataVecTemp[vAt][o];

              bool gotErrorsClose=0;
              bool gotErrorsHiLo=0;
              bool gotErrorsVol=0;

           if(filterSet.range){
              if(oo.HI<=0 ){
                 dataProb.HI0++;
                  gotErrorsHiLo=1;
              }if(oo.LO<=0){
                  dataProb.LO0++;
                  gotErrorsHiLo=1;
              }if(oo.CLOSE<=0){
                  dataProb.CLOSE0++;
                  gotErrorsClose=1;
              }if(oo.HI < oo.LO){
                  dataProb.HIuLO++;
                  gotErrorsHiLo=1;
              }if(oo.CLOSE > oo.HI){
                  dataProb.CLOSEoHI++;
                  gotErrorsClose=1;
              }if(oo.CLOSE < oo.LO){
                  dataProb.CLOSEuLO++;
                  gotErrorsClose=1;
              }
              if(oo.CLOSE == data_struct_max){
                   dataProb.closeMax++;
                   gotErrorsClose=1;
               }if(oo.HI == data_struct_max){
                   dataProb.hiMax++;
                   gotErrorsHiLo=1;
               }if(oo.LO == data_struct_max){
                   dataProb.loMax++;
                   gotErrorsHiLo=1;
               }
          }

           if(filterSet.drop){
               if(o<dataVecTemp[vAt].size()-1){
                   data_struct ooo1=dataVecTemp[vAt][o+1];
                   if(ooo1.CLOSE>oo.CLOSE*2){
                       dataProb.tooHighDrop++;
                       gotErrorsClose=1;
                   }
                   if(ooo1.HI>oo.HI*2){
                       dataProb.tooHighDrop++;
                       gotErrorsHiLo=1;
                   }
                   if(ooo1.LO>oo.LO*2){
                       dataProb.tooHighDrop++;
                       gotErrorsHiLo=1;
                   }
               }
           }

               if(o>1){
                   data_struct oo1=dataVecTemp[vAt][o-1];
                   data_struct oo2=dataVecTemp[vAt][o-2];


               if(filterSet.overUnder){

                   if((oo.CLOSE == oo1.CLOSE && oo.CLOSE == oo2.CLOSE) ){
                       dataProb.closeSame++;
                       gotErrorsClose=1;
                   }if((oo.HI == oo1.HI && oo.HI == oo2.HI)){
                       dataProb.hiSame++;
                       gotErrorsHiLo=1;
                   }if((oo.LO == oo1.LO && oo.LO == oo2.LO) ){
                       dataProb.loSame++;
                       gotErrorsHiLo=1;
                   }


                   if((oo.CLOSE == oo.HI && oo.CLOSE == oo1.HI) ){
                                      dataProb.closeIsHi++;
                                      gotErrorsClose=1;
                                  }
                   if((oo.CLOSE == oo.LO && oo.CLOSE == oo1.LO) ){
                                      dataProb.closeIsLo++;
                                      gotErrorsClose=1;
                                  }
    }


               if(filterSet.barSimilarity){
                   double dif1=oo.HI-oo.LO;
                   double dif2=oo1.HI-oo1.LO;

                   double dif=max(dif1,dif2);

                   //how close bars should be


                   double maxSlide=dif*filterSet.maxAllowedBarsSimilarity;

                   bool hiClose=0;
                   bool loClose=0;
                   bool closeClose=0;


                   if(fabs(oo.HI-oo1.HI)<maxSlide){
                            hiClose=1;
                            if(fabs(oo.CLOSE-oo1.HI)<maxSlide){
                                   closeClose=1;
                            }
                   }
                   if(fabs(oo.LO-oo1.LO)<maxSlide){
                            loClose=1;
                            if(fabs(oo.CLOSE-oo1.LO)<maxSlide){
                                   closeClose=1;
                            }
                   }

                   if((hiClose&&loClose)){
                       dataProb.tooCloseRatioProblem++;
                       gotErrorsHiLo=1;
                   }
                   if((hiClose&&closeClose)||(loClose&&closeClose)){
                       dataProb.tooCloseRatioProblem++;
                       gotErrorsClose=1;
                   }

    }

               }

               if(filterSet.drop2){

                   if(oo.LO/oo.HI < filterSet.allowedDrop2){
                                  dataProb.badLoHiRatio++;
                                  gotErrorsHiLo=1;
                   }
               }



       if(gotErrorsVol||gotErrorsHiLo||gotErrorsClose){
           dataProb.totalProblematicIndexes++;


           if(gotErrorsClose)
           dataProb.totalCloseErrors++;
           else
               dataVecTemp[vAt][o].closeOk=1;


           if(gotErrorsHiLo)
           dataProb.totalHiLoErrors++;
           else
           dataVecTemp[vAt][o].hiLoOk=1;

       }else{
           dataProb.totalOkData++;
           dataVecTemp[vAt][o].hiLoOk=1;
           dataVecTemp[vAt][o].closeOk=1;

       }

//NB why is temp vec even needed?, cant remember, check if time

       if(!gotErrorsHiLo){

           if(dataVecLo[vAt]>dataVecTemp[vAt][o].LO)
               dataVecLo[vAt]=dataVecTemp[vAt][o].LO;

           if(dataVecHi[vAt]>dataVecTemp[vAt][o].HI)
               dataVecHi[vAt]=dataVecTemp[vAt][o].HI;

           }
          }
      }


}

void File_Load_Class::printDataProblems(File_Load_Class::dataProblems &dataProb)
{

    if(dataProb.HI0!=0)
            errorLine("Total HI0  "+CONVERT.numTo_B_M_K_String(dataProb.HI0)+"  "+CONVERT.GetPercentString(    dataProb.HI0  ,dataProb.totalData));

    if(dataProb.LO0!=0)
            errorLine("Total LO0  "+CONVERT.numTo_B_M_K_String(dataProb.LO0)+"  "+CONVERT.GetPercentString(     dataProb.LO0 ,dataProb.totalData));

    if(dataProb.CLOSE0!=0)
            errorLine("Total CLOSE0  "+CONVERT.numTo_B_M_K_String(dataProb.CLOSE0)+"  "+CONVERT.GetPercentString(   dataProb.CLOSE0   ,dataProb.totalData));

    if(dataProb.HIuLO!=0)
            errorLine("Total HI<=LO  "+CONVERT.numTo_B_M_K_String(dataProb.HIuLO)+"  "+CONVERT.GetPercentString(   dataProb.HIuLO   ,dataProb.totalData));

    if(dataProb.CLOSEoHI!=0)
            errorLine("Total CLOSE>HI  "+CONVERT.numTo_B_M_K_String(dataProb.CLOSEoHI)+"  "+CONVERT.GetPercentString(   dataProb.CLOSEoHI   ,dataProb.totalData));

    if(dataProb.CLOSEuLO!=0)
            errorLine("Total CLOSE<LO  "+CONVERT.numTo_B_M_K_String(dataProb.CLOSEuLO)+"  "+CONVERT.GetPercentString( dataProb.CLOSEuLO     ,dataProb.totalData));

    if(dataProb.closeMax!=0)
            errorLine("Total CLOSE Max  "+CONVERT.numTo_B_M_K_String(dataProb.closeMax)+"  "+CONVERT.GetPercentString( dataProb.closeMax     ,dataProb.totalData));

    if(dataProb.loMax!=0)
            errorLine("Total LO Max  "+CONVERT.numTo_B_M_K_String(dataProb.loMax)+"  "+CONVERT.GetPercentString(    dataProb.loMax  ,dataProb.totalData));

    if(dataProb.hiMax!=0)
            errorLine("Total HI Max  "+CONVERT.numTo_B_M_K_String(dataProb.hiMax)+"  "+CONVERT.GetPercentString(  dataProb.hiMax    ,dataProb.totalData));

    if(dataProb.hiSame!=0)
            errorLine("hiSame  "+CONVERT.numTo_B_M_K_String(dataProb.hiSame)+"  "+CONVERT.GetPercentString(   dataProb.hiSame   ,dataProb.totalData));
    if(dataProb.loSame!=0)
            errorLine("loSame  "+CONVERT.numTo_B_M_K_String(dataProb.loSame)+"  "+CONVERT.GetPercentString( dataProb.loSame     ,dataProb.totalData));

    if(dataProb.closeSame!=0)
            errorLine("closeSame  "+CONVERT.numTo_B_M_K_String(dataProb.closeSame)+"  "+CONVERT.GetPercentString(dataProb.closeSame,dataProb.totalData));




    if(dataProb.closeIsHi!=0)
            errorLine("closeIsHi  "+CONVERT.numTo_B_M_K_String(dataProb.closeIsHi)+"  "+CONVERT.GetPercentString(dataProb.closeIsHi,dataProb.totalData));

    if(dataProb.closeIsLo!=0)
            errorLine("closeIsLo  "+CONVERT.numTo_B_M_K_String(dataProb.closeIsLo)+"  "+CONVERT.GetPercentString(dataProb.closeIsLo,dataProb.totalData));

    if(dataProb.badLoHiRatio!=0)
            errorLine("badLoHiRatio  "+CONVERT.numTo_B_M_K_String(dataProb.badLoHiRatio)+"  "+CONVERT.GetPercentString(dataProb.badLoHiRatio,dataProb.totalData));

    if(dataProb.tooCloseRatioProblem!=0)
            errorLine("tooCloseRatioProblem  "+CONVERT.numTo_B_M_K_String(dataProb.tooCloseRatioProblem)+"  "+CONVERT.GetPercentString(dataProb.tooCloseRatioProblem,dataProb.totalData));

    if(dataProb.tooHighDrop!=0)
            errorLine("closeTooHighDrop  "+CONVERT.numTo_B_M_K_String(dataProb.tooHighDrop)+"  "+CONVERT.GetPercentString(dataProb.tooHighDrop,dataProb.totalData));


    if(dataProb.totalHiLoErrors!=0)
     errorLine("Total hiLo Problems..."+CONVERT.numTo_B_M_K_String(dataProb.totalHiLoErrors)+"  "+CONVERT.GetPercentString(dataProb.totalHiLoErrors,dataProb.totalData));
    if(dataProb.totalOpenErrors!=0)
     errorLine("Total open Problems..."+CONVERT.numTo_B_M_K_String(dataProb.totalOpenErrors)+"  "+CONVERT.GetPercentString(dataProb.totalOpenErrors,dataProb.totalData));
    if(dataProb.totalCloseErrors!=0)
     errorLine("Total close Problems..."+CONVERT.numTo_B_M_K_String(dataProb.totalOpenErrors)+"  "+CONVERT.GetPercentString(dataProb.totalCloseErrors,dataProb.totalData));
    if(dataProb.totalVolErrors!=0)
     errorLine("Total volume Problems..."+CONVERT.numTo_B_M_K_String(dataProb.totalVolErrors)+"  "+CONVERT.GetPercentString(dataProb.totalVolErrors,dataProb.totalData));





   if(dataProb.totalProblematicIndexes!=0)
    errorLine("TotalProblems..."+CONVERT.numTo_B_M_K_String(dataProb.totalProblematicIndexes)+"  "+CONVERT.GetPercentString(dataProb.totalProblematicIndexes,dataProb.totalData));

   infoLine("totalOKData..." + CONVERT.numTo_B_M_K_String(dataProb.totalOkData)+"  "+CONVERT.GetPercentString(dataProb.totalOkData,dataProb.totalData));
   infoLine("totalData..." + CONVERT.numTo_B_M_K_String(dataProb.totalData));

}
