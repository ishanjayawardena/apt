#include<iostream>

using namespace std;

#include <locale.h>
#include <langinfo.h>

#include <apt-pkg/strutl.h>

#include "private-output.h"
#include "private-cachefile.h"

#include <apti18n.h>

std::ostream c0out(0);
std::ostream c1out(0);
std::ostream c2out(0);
std::ofstream devnull("/dev/null");
unsigned int ScreenWidth = 80 - 1; /* - 1 for the cursor */


bool InitOutput()
{
   c0out.rdbuf(cout.rdbuf());
   c1out.rdbuf(cout.rdbuf());
   c2out.rdbuf(cout.rdbuf());
   if (_config->FindI("quiet",0) > 0)
      c0out.rdbuf(devnull.rdbuf());
   if (_config->FindI("quiet",0) > 1)
      c1out.rdbuf(devnull.rdbuf());
   return true;
}

// ShowList - Show a list						/*{{{*/
// ---------------------------------------------------------------------
/* This prints out a string of space separated words with a title and 
   a two space indent line wraped to the current screen width. */
bool ShowList(ostream &out,string Title,string List,string VersionsList)
{
   if (List.empty() == true)
      return true;
   // trim trailing space
   int NonSpace = List.find_last_not_of(' ');
   if (NonSpace != -1)
   {
      List = List.erase(NonSpace + 1);
      if (List.empty() == true)
	 return true;
   }

   // Acount for the leading space
   int ScreenWidth = ::ScreenWidth - 3;
      
   out << Title << endl;
   string::size_type Start = 0;
   string::size_type VersionsStart = 0;
   while (Start < List.size())
   {
      if(_config->FindB("APT::Get::Show-Versions",false) == true &&
         VersionsList.size() > 0) {
         string::size_type End;
         string::size_type VersionsEnd;
         
         End = List.find(' ',Start);
         VersionsEnd = VersionsList.find('\n', VersionsStart);

         out << "   " << string(List,Start,End - Start) << " (" << 
            string(VersionsList,VersionsStart,VersionsEnd - VersionsStart) << 
            ")" << endl;

	 if (End == string::npos || End < Start)
	    End = Start + ScreenWidth;

         Start = End + 1;
         VersionsStart = VersionsEnd + 1;
      } else {
         string::size_type End;

         if (Start + ScreenWidth >= List.size())
            End = List.size();
         else
            End = List.rfind(' ',Start+ScreenWidth);

         if (End == string::npos || End < Start)
            End = Start + ScreenWidth;
         out << "  " << string(List,Start,End - Start) << endl;
         Start = End + 1;
      }
   }   

   return false;
}
									/*}}}*/
// ShowBroken - Debugging aide						/*{{{*/
// ---------------------------------------------------------------------
/* This prints out the names of all the packages that are broken along
   with the name of each each broken dependency and a quite version 
   description.
   
   The output looks like:
 The following packages have unmet dependencies:
     exim: Depends: libc6 (>= 2.1.94) but 2.1.3-10 is to be installed
           Depends: libldap2 (>= 2.0.2-2) but it is not going to be installed
           Depends: libsasl7 but it is not going to be installed   
 */
void ShowBroken(ostream &out,CacheFile &Cache,bool Now)
{
   if (Cache->BrokenCount() == 0)
      return;

   out << _("The following packages have unmet dependencies:") << endl;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      
      if (Now == true)
      {
	 if (Cache[I].NowBroken() == false)
	    continue;
      }
      else
      {
	 if (Cache[I].InstBroken() == false)
	    continue;
      }
      
      // Print out each package and the failed dependencies
      out << " " << I.FullName(true) << " :";
      unsigned const Indent = I.FullName(true).size() + 3;
      bool First = true;
      pkgCache::VerIterator Ver;
      
      if (Now == true)
	 Ver = I.CurrentVer();
      else
	 Ver = Cache[I].InstVerIter(Cache);
      
      if (Ver.end() == true)
      {
	 out << endl;
	 continue;
      }
      
      for (pkgCache::DepIterator D = Ver.DependsList(); D.end() == false;)
      {
	 // Compute a single dependency element (glob or)
	 pkgCache::DepIterator Start;
	 pkgCache::DepIterator End;
	 D.GlobOr(Start,End); // advances D

	 if (Cache->IsImportantDep(End) == false)
	    continue;
	 
	 if (Now == true)
	 {
	    if ((Cache[End] & pkgDepCache::DepGNow) == pkgDepCache::DepGNow)
	       continue;
	 }
	 else
	 {
	    if ((Cache[End] & pkgDepCache::DepGInstall) == pkgDepCache::DepGInstall)
	       continue;
	 }
	 
	 bool FirstOr = true;
	 while (1)
	 {
	    if (First == false)
	       for (unsigned J = 0; J != Indent; J++)
		  out << ' ';
	    First = false;

	    if (FirstOr == false)
	    {
	       for (unsigned J = 0; J != strlen(End.DepType()) + 3; J++)
		  out << ' ';
	    }
	    else
	       out << ' ' << End.DepType() << ": ";
	    FirstOr = false;
	    
	    out << Start.TargetPkg().FullName(true);
	 
	    // Show a quick summary of the version requirements
	    if (Start.TargetVer() != 0)
	       out << " (" << Start.CompType() << " " << Start.TargetVer() << ")";
	    
	    /* Show a summary of the target package if possible. In the case
	       of virtual packages we show nothing */	 
	    pkgCache::PkgIterator Targ = Start.TargetPkg();
	    if (Targ->ProvidesList == 0)
	    {
	       out << ' ';
	       pkgCache::VerIterator Ver = Cache[Targ].InstVerIter(Cache);
	       if (Now == true)
		  Ver = Targ.CurrentVer();
	       	    
	       if (Ver.end() == false)
	       {
		  if (Now == true)
		     ioprintf(out,_("but %s is installed"),Ver.VerStr());
		  else
		     ioprintf(out,_("but %s is to be installed"),Ver.VerStr());
	       }	       
	       else
	       {
		  if (Cache[Targ].CandidateVerIter(Cache).end() == true)
		  {
		     if (Targ->ProvidesList == 0)
			out << _("but it is not installable");
		     else
			out << _("but it is a virtual package");
		  }		  
		  else
		     out << (Now?_("but it is not installed"):_("but it is not going to be installed"));
	       }	       
	    }
	    
	    if (Start != End)
	       out << _(" or");
	    out << endl;
	    
	    if (Start == End)
	       break;
	    ++Start;
	 }	 
      }	    
   }   
}
									/*}}}*/
// ShowNew - Show packages to newly install				/*{{{*/
// ---------------------------------------------------------------------
/* */
void ShowNew(ostream &out,CacheFile &Cache)
{
   /* Print out a list of packages that are going to be installed extra
      to what the user asked */
   string List;
   string VersionsList;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      if (Cache[I].NewInstall() == true) {
         List += I.FullName(true) + " ";
         VersionsList += string(Cache[I].CandVersion) + "\n";
      }
   }
   
   ShowList(out,_("The following NEW packages will be installed:"),List,VersionsList);
}
									/*}}}*/
// ShowDel - Show packages to delete					/*{{{*/
// ---------------------------------------------------------------------
/* */
void ShowDel(ostream &out,CacheFile &Cache)
{
   /* Print out a list of packages that are going to be removed extra
      to what the user asked */
   string List;
   string VersionsList;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      if (Cache[I].Delete() == true)
      {
	 if ((Cache[I].iFlags & pkgDepCache::Purge) == pkgDepCache::Purge)
	    List += I.FullName(true) + "* ";
	 else
	    List += I.FullName(true) + " ";
     
     VersionsList += string(Cache[I].CandVersion)+ "\n";
      }
   }
   
   ShowList(out,_("The following packages will be REMOVED:"),List,VersionsList);
}
									/*}}}*/
// ShowKept - Show kept packages					/*{{{*/
// ---------------------------------------------------------------------
/* */
void ShowKept(ostream &out,CacheFile &Cache)
{
   string List;
   string VersionsList;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {	 
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      
      // Not interesting
      if (Cache[I].Upgrade() == true || Cache[I].Upgradable() == false ||
	  I->CurrentVer == 0 || Cache[I].Delete() == true)
	 continue;
      
      List += I.FullName(true) + " ";
      VersionsList += string(Cache[I].CurVersion) + " => " + Cache[I].CandVersion + "\n";
   }
   ShowList(out,_("The following packages have been kept back:"),List,VersionsList);
}
									/*}}}*/
// ShowUpgraded - Show upgraded packages				/*{{{*/
// ---------------------------------------------------------------------
/* */
void ShowUpgraded(ostream &out,CacheFile &Cache)
{
   string List;
   string VersionsList;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      
      // Not interesting
      if (Cache[I].Upgrade() == false || Cache[I].NewInstall() == true)
	 continue;

      List += I.FullName(true) + " ";
      VersionsList += string(Cache[I].CurVersion) + " => " + Cache[I].CandVersion + "\n";
   }
   ShowList(out,_("The following packages will be upgraded:"),List,VersionsList);
}
									/*}}}*/
// ShowDowngraded - Show downgraded packages				/*{{{*/
// ---------------------------------------------------------------------
/* */
bool ShowDowngraded(ostream &out,CacheFile &Cache)
{
   string List;
   string VersionsList;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      
      // Not interesting
      if (Cache[I].Downgrade() == false || Cache[I].NewInstall() == true)
	 continue;

      List += I.FullName(true) + " ";
      VersionsList += string(Cache[I].CurVersion) + " => " + Cache[I].CandVersion + "\n";
   }
   return ShowList(out,_("The following packages will be DOWNGRADED:"),List,VersionsList);
}
									/*}}}*/
// ShowHold - Show held but changed packages				/*{{{*/
// ---------------------------------------------------------------------
/* */
bool ShowHold(ostream &out,CacheFile &Cache)
{
   string List;
   string VersionsList;
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      if (Cache[I].InstallVer != (pkgCache::Version *)I.CurrentVer() &&
          I->SelectedState == pkgCache::State::Hold) {
         List += I.FullName(true) + " ";
		 VersionsList += string(Cache[I].CurVersion) + " => " + Cache[I].CandVersion + "\n";
      }
   }

   return ShowList(out,_("The following held packages will be changed:"),List,VersionsList);
}
									/*}}}*/
// ShowEssential - Show an essential package warning			/*{{{*/
// ---------------------------------------------------------------------
/* This prints out a warning message that is not to be ignored. It shows
   all essential packages and their dependents that are to be removed. 
   It is insanely risky to remove the dependents of an essential package! */
bool ShowEssential(ostream &out,CacheFile &Cache)
{
   string List;
   string VersionsList;
   bool *Added = new bool[Cache->Head().PackageCount];
   for (unsigned int I = 0; I != Cache->Head().PackageCount; I++)
      Added[I] = false;
   
   for (unsigned J = 0; J < Cache->Head().PackageCount; J++)
   {
      pkgCache::PkgIterator I(Cache,Cache.List[J]);
      if ((I->Flags & pkgCache::Flag::Essential) != pkgCache::Flag::Essential &&
	  (I->Flags & pkgCache::Flag::Important) != pkgCache::Flag::Important)
	 continue;
      
      // The essential package is being removed
      if (Cache[I].Delete() == true)
      {
	 if (Added[I->ID] == false)
	 {
	    Added[I->ID] = true;
	    List += I.FullName(true) + " ";
        //VersionsList += string(Cache[I].CurVersion) + "\n"; ???
	 }
      }
      else
	 continue;

      if (I->CurrentVer == 0)
	 continue;

      // Print out any essential package depenendents that are to be removed
      for (pkgCache::DepIterator D = I.CurrentVer().DependsList(); D.end() == false; ++D)
      {
	 // Skip everything but depends
	 if (D->Type != pkgCache::Dep::PreDepends &&
	     D->Type != pkgCache::Dep::Depends)
	    continue;
	 
	 pkgCache::PkgIterator P = D.SmartTargetPkg();
	 if (Cache[P].Delete() == true)
	 {
	    if (Added[P->ID] == true)
	       continue;
	    Added[P->ID] = true;
	    
	    char S[300];
	    snprintf(S,sizeof(S),_("%s (due to %s) "),P.FullName(true).c_str(),I.FullName(true).c_str());
	    List += S;
        //VersionsList += "\n"; ???
	 }	 
      }      
   }
   
   delete [] Added;
   return ShowList(out,_("WARNING: The following essential packages will be removed.\n"
			 "This should NOT be done unless you know exactly what you are doing!"),List,VersionsList);
}

									/*}}}*/
// Stats - Show some statistics						/*{{{*/
// ---------------------------------------------------------------------
/* */
void Stats(ostream &out,pkgDepCache &Dep)
{
   unsigned long Upgrade = 0;
   unsigned long Downgrade = 0;
   unsigned long Install = 0;
   unsigned long ReInstall = 0;
   for (pkgCache::PkgIterator I = Dep.PkgBegin(); I.end() == false; ++I)
   {
      if (Dep[I].NewInstall() == true)
	 Install++;
      else
      {
	 if (Dep[I].Upgrade() == true)
	    Upgrade++;
	 else
	    if (Dep[I].Downgrade() == true)
	       Downgrade++;
      }
      
      if (Dep[I].Delete() == false && (Dep[I].iFlags & pkgDepCache::ReInstall) == pkgDepCache::ReInstall)
	 ReInstall++;
   }   

   ioprintf(out,_("%lu upgraded, %lu newly installed, "),
	    Upgrade,Install);
   
   if (ReInstall != 0)
      ioprintf(out,_("%lu reinstalled, "),ReInstall);
   if (Downgrade != 0)
      ioprintf(out,_("%lu downgraded, "),Downgrade);

   ioprintf(out,_("%lu to remove and %lu not upgraded.\n"),
	    Dep.DelCount(),Dep.KeepCount());
   
   if (Dep.BadCount() != 0)
      ioprintf(out,_("%lu not fully installed or removed.\n"),
	       Dep.BadCount());
}
									/*}}}*/
// YnPrompt - Yes No Prompt.						/*{{{*/
// ---------------------------------------------------------------------
/* Returns true on a Yes.*/
bool YnPrompt(bool Default)
{
   if (_config->FindB("APT::Get::Assume-Yes",false) == true)
   {
      c1out << _("Y") << endl;
      return true;
   }
   else if (_config->FindB("APT::Get::Assume-No",false) == true)
   {
      c1out << _("N") << endl;
      return false;
   }

   char response[1024] = "";
   cin.getline(response, sizeof(response));

   if (!cin)
      return false;

   if (strlen(response) == 0)
      return Default;

   regex_t Pattern;
   int Res;

   Res = regcomp(&Pattern, nl_langinfo(YESEXPR),
                 REG_EXTENDED|REG_ICASE|REG_NOSUB);

   if (Res != 0) {
      char Error[300];        
      regerror(Res,&Pattern,Error,sizeof(Error));
      return _error->Error(_("Regex compilation error - %s"),Error);
   }
   
   Res = regexec(&Pattern, response, 0, NULL, 0);
   if (Res == 0)
      return true;
   return false;
}
									/*}}}*/
// AnalPrompt - Annoying Yes No Prompt.					/*{{{*/
// ---------------------------------------------------------------------
/* Returns true on a Yes.*/
bool AnalPrompt(const char *Text)
{
   char Buf[1024];
   cin.getline(Buf,sizeof(Buf));
   if (strcmp(Buf,Text) == 0)
      return true;
   return false;
}
									/*}}}*/