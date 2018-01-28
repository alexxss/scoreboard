//please use C++11!!
#define __USE_MINGW_ANSI_STDIO 0
#include <stdio.h>
#include<iostream>
#include<vector>
#include<map>
#include<iomanip>
#include<fstream>
using namespace std;

int cycle = 0;
map<string,int> CycleNo = {{"L.D",1},{"ADD.D",2},{"SUB.D",2},{"MUL.D",10},{"DIV.D",40}};
vector<int> IssueOrder;
vector<int> ReadOrder;
vector<int> WriteOrder;

class FUStatus{
public:
    int time;
    bool busy;
    string op, Fi, Fj, Fk, Qj, Qk;
    bool Rj, Rk;

    FUStatus(){
        time = -1;
        busy = false;
        op = Fi = Fj = Fk = Qj = Qk = "";
        Rj = Rk = false;
    }

};

class RegisterResultStatus {
public:

    map<string,string> FU;

    RegisterResultStatus(){
        string index;
        for (int i = 0; i <= 30; i++){
            index = 'F' + std::to_string(i);
        }
    }
};

class InstructionStatus { //each instruction status is an object
public:

    string op;
    string Fi;
    string Fj;
    string Fk;
    string FU;
    int Issue;
    int ReadOp;
    int ExecComp;
    int WriteResult;

    InstructionStatus(string instruction){
        Issue = ReadOp = ExecComp = WriteResult = 0;
        FU = "";
        int i = instruction.find(' ');
        op = instruction.substr(0,i); //cut op
        instruction = instruction.substr(i+1);
 //       cout<<"i="<<i<<", instruction="<<instruction<<endl;

        i = instruction.find(',');
        Fi = instruction.substr(0,i); //cut Fi
        instruction = instruction.substr(i+2);


        if (op == "L.D"){
            i = instruction.find('(');
            Fj = instruction.substr(0,i); //cut Fj
            i++;
            Fk = instruction.substr(i,instruction.find(')')-i); //cut Fk
//            cout<<"i="<<i<<", instruction.find(')')="<<instruction.find(')')<<endl;
        }else{
            i = instruction.find(',');
            Fj = instruction.substr(0,i); //cut Fj
            Fk = instruction.substr(i+2); //cut Fk
        }

    }

    void printIS(){ //輸出這條指令的Instruction Status
        cout<<"op="<<op<<", Fi="<<Fi<<", Fj="<<Fj<<", Fk="<<Fk<<endl;
    }
};

vector<InstructionStatus> IS;
map<string,FUStatus> FUStat = {{"Integer",FUStatus()},{"Mult1",FUStatus()},{"Mult2",FUStatus()},{"Add",FUStatus()},{"Divide",FUStatus()}};
RegisterResultStatus RRS = RegisterResultStatus();

/*---------------"Write Result" related functions----------------*/
bool checkWrite(string& FU){
    if (FUStat[FU].time == 0){ //countdown done?
        string dest = FUStat[FU].Fi;
        for(auto& fustat : FUStat){
            // if all other Fu is not waiting to read from dest, or has already read from dest
            FUStatus& fu = fustat.second;
            if(!((fu.Fj != dest)||(fu.Rj==false) && ((fu.Fk != dest)||(fu.Rk == false)))){
                //cannot write!
                cout<<"fu.Fj="<<fu.Fj<<endl;
                return false;
            }
        }
        return true;
    } else  //cd not done, cannot write
        return false;
}

void doWrite(string& FU,int& ind){
    int I = WriteOrder[ind];
    //mark ready for whoever is waiting
    for(auto& w : FUStat){
        FUStatus& waiting = w.second;
        if(waiting.Qj == FU) {
            waiting.Qj = "";
            waiting.Rj = true;
        }
        if(waiting.Qk == FU) {
            waiting.Qk = "";
            waiting.Rk = true;
        }
    }
    //reset result reg status
    string dest = IS[I].Fi;
    RRS.FU[dest] = "";
    //clear FU
    FUStat.erase(FU);
    FUStat[FU] = FUStatus();
    //mark cycle in IS
    IS[I].WriteResult = cycle;
    //remove from writeOrder
    WriteOrder.erase(WriteOrder.begin()+ind);
    ind--;
}

void write(){
    for(int ind = 0; ind < WriteOrder.size(); ind++){
        int I = WriteOrder[ind];
        if (IS[I].ExecComp < cycle && IS[I].ExecComp != 0){// check for write back if exec completed
            string FU = IS[I].FU;
            if (checkWrite(FU)){ //can write?
                doWrite(FU,ind);
            }
        }
    }
}

/*---------------"Exec Comp" related functions-------------------*/
void exec(){
    // time-- (if time = 1, do execComp'd)
    for(int I = 0; I < IS.size(); I++){
        if((IS[I].ReadOp < cycle && IS[I].ReadOp != 0) && IS[I].ExecComp==0){ //is counting down
            string FU = IS[I].FU;
            FUStat[FU].time--;
            if(FUStat[FU].time == 0) IS[I].ExecComp=cycle; //count down complete, mark IS
        }
    }
}

/*---------------"Read Opr" related functions-------------------*/
void readOp(){
    for(int ind = 0; ind < ReadOrder.size(); ind++){
        int I = ReadOrder[ind];
//        cout<<"I="<<I<<",op="<<IS[I].op<<",Fi="<<IS[I].Fi<<endl;
        if(IS[I].Issue < cycle && IS[I].Issue != 0){
            //check if can read
            string FU = IS[I].FU;
            if(FUStat[FU].Rj && FUStat[FU].Rk) {
                //read
                FUStat[FU].Rj = FUStat[FU].Rk = false;
                FUStat[FU].Qj = FUStat[FU].Qk = "";
                //start countdown
                FUStat[FU].time = CycleNo[FUStat[FU].op];
                //mark IS
                IS[I].ReadOp = cycle;
                //remove from readorder
                ReadOrder.erase(ReadOrder.begin()+ind);
                ind--;

            }
        }
    }
}

/*------------------"Issue" related functions--------------------*/

bool destOK(string& dest){
    if(RRS.FU[dest]=="") return true;
    else return false;
}

bool getFU(string& op, string& FU){
    if(op=="L.D"){
       if(!FUStat["Integer"].busy) FU = "Integer";
       else return false;
    }else if (op == "ADD.D" || op == "SUB.D"){
        if(!FUStat["Add"].busy) FU = "Add";
       else return false;
    }else if (op == "MUL.D"){
        if(!FUStat["Mult1"].busy) FU = "Mult1";
        else if(!FUStat["Mult2"].busy) FU = "Mult2";
       else return false;
    }else if (op == "DIV.D"){
        if(!FUStat["Divide"].busy) FU = "Divide";
       else return false;
    }else{
        cout<<"op error!!"<<endl;
        return false;
    }
//    cout<<"FU="<<FU<<endl;
//    if(FUStat[FU].busy)cout<<"FU Busy."; else cout<<"FU Free."; cout<<endl;
    return true;
}
void issue(){
    /* check if can issue */
    if(IssueOrder.size()==0)return;
    int I = IssueOrder.front();
    string op = IS[I].op;
    string dest = IS[I].Fi;
    string S1 = IS[I].Fj;
    string S2 = IS[I].Fk;
    //can issue?
    bool issue = false;
    //determine FU
    string FU="";
    if(getFU(op,FU) && destOK(dest)) //!Busy FU && !waiting[Fi]
    {
           //bookkeep
            FUStat[FU].busy = true;
            FUStat[FU].op = op;
            FUStat[FU].Fi = dest;
            if(op!="L.D"){
                FUStat[FU].Fj = S1;
            }
            FUStat[FU].Fk = S2;
            FUStat[FU].Qj = RRS.FU[S1];
            FUStat[FU].Qk = RRS.FU[S2];
            if(RRS.FU[S1] == "")
                FUStat[FU].Rj = true;
            if(RRS.FU[S2] == "")
                FUStat[FU].Rk = true;
            RRS.FU[dest] = FU;
            //mark IS
            IS[I].Issue = cycle;
            IS[I].FU = FU;
            //remove from issue order
            IssueOrder.erase(IssueOrder.begin());
    }
}

/*----------------------print------------------------------------*/
void print(){
    cout<<"Cycle #"<<cycle<<"-----------------------------------------------------------------------"<<endl;
    //print IS
    cout<<"=====INSTRUCTION STATUS====="<<endl;
    cout<<setw(6)<<"Ins"<<setw(4)<<" "<<setw(4)<<"j"<<setw(4)<<"k";
    cout<<setw(6)<<"Issue"<<setw(5)<<"Read"<<setw(5)<<"Exec"<<setw(6)<<"Write"<<endl;
    for(auto& is : IS){
        cout<<setw(6)<<is.op<<setw(4)<<is.Fi<<setw(4)<<is.Fj<<setw(4)<<is.Fk;
        cout<<setw(6)<<is.Issue<<setw(5)<<is.ReadOp<<setw(5)<<is.ExecComp<<setw(6)<<is.WriteResult<<endl;
        cout<<endl;
    }
    //print FU
    cout<<"==========FU STATUS========="<<endl;
    cout<<setw(5)<<"Time"<<setw(8)<<"Name"<<setw(5)<<"Busy"<<setw(6)<<"Op"<<setw(4)<<"Fi"<<setw(4)<<"Fj"<<setw(4)<<"Fk"<<setw(8)<<"Qj"<<setw(8)<<"Qk"<<setw(4)<<"Rj"<<setw(4)<<"Rk"<<endl;
    for(auto& fus : FUStat){
        string name = fus.first;
        FUStatus fu = fus.second;
        if(fu.time!=-1) cout<<setw(5)<<fu.time; else cout<<setw(5)<<" ";
        cout<<setw(8)<<name;
        if(fu.busy)cout<<setw(5)<<"Yes"; else cout<<setw(5)<<"No";
        cout<<setw(6)<<fu.op;
        cout<<setw(4)<<fu.Fi;
        cout<<setw(4)<<fu.Fj;
        cout<<setw(4)<<fu.Fk;
        cout<<setw(8)<<fu.Qj;
        cout<<setw(8)<<fu.Qk;
        if(fu.Rj)cout<<setw(4)<<"Yes"; else cout<<setw(4)<<"No";
        if(fu.Rk)cout<<setw(4)<<"Yes"; else cout<<setw(4)<<"No";
        cout<<endl<<endl;
    }
    //print RRS
    cout<<"======REG RESULT STATUS====="<<endl;
    //print title
    cout<<setw(6)<<"Clock";
    for(auto& fu : RRS.FU)
        if(fu.second != "") cout<<setw(8)<<fu.first; //print FU name if not empty
    cout<<endl;
    //print content
    cout<<setw(6)<<cycle;
    for(auto& fu : RRS.FU)
        if(fu.second != "") cout<<setw(8)<<fu.second;
    cout<<endl;

    cout<<endl<<endl;
}

int main(){
    /* --- initialize --- */
    string s;
/*
    FUStat["Integer"] = FUStatus();
    FUStat["Mult1"] = FUStatus();
    FUStat["Mult2"] = FUStatus();
    FUStat["Add"] = FUStatus();
    FUStat["Divide"] = FUStatus();*/
    /* -------------------*/

    /* ----read input-----*/
    ifstream input;
    input.open("input.txt");
    if(input.is_open()){
            while(getline(input,s)){
                InstructionStatus iS(s);
                IS.push_back(iS);
        //        iS.printIS();
            }
    }
    input.close();

    /* ------initialize---*/
    int val;
    for(int i = 0; i < IS.size(); i++){
        val = i;
        IssueOrder.push_back(val);
        ReadOrder.push_back(val);
        WriteOrder.push_back(val);
    }

    print();
    /* -----cycle start---*/

    for(int k = 0; k < 70; k++){
        cycle++;/*
        write();
        exec();
        readOp();
        issue();*/
        issue();
        readOp();
        exec();
        write();
        print();
        if(WriteOrder.size()==0)break;
    }
}
