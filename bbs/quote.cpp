/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*             Copyright (C)1998-2017, WWIV Software Services             */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/*                                                                        */
/**************************************************************************/

#include <memory>
#include <string>

#include "bbs/bbsutl.h"
#include "bbs/input.h"
#include "bbs/keycodes.h"
#include "bbs/printfile.h"
#include "bbs/bbs.h"
#include "bbs/bbsutl2.h"
#include "bbs/com.h"
#include "bbs/message_file.h"
#include "bbs/pause.h"
#include "bbs/vars.h"
#include "bbs/utility.h"
#include "core/strings.h"
#include "core/textfile.h"
#include "sdk/datetime.h"
#include "sdk/filenames.h"

//
// Local function prototypes
//
char *GetQuoteInitials();

#define LINELEN 79
#define PFXCOL 2
#define QUOTECOL 0

#define WRTPFX {file.WriteFormatted("\x3%c",PFXCOL+48);if (tf==1)\
                cp=file.WriteBinary(pfx,pfxlen-1);\
                else cp=file.WriteBinary(pfx,pfxlen);\
                file.WriteFormatted("\x3%c",cc);}
#define NL {if (!cp) {file.WriteFormatted("\x3%c",PFXCOL+48);\
            file.WriteBinary(pfx,pfxlen);} if (ctlc) file.WriteBinary("0",1);\
            file.WriteBinary("\r\n",2);cp=ns=ctlc=0;}
#define FLSH {if (ss1) {if (cp && (l3+cp>=linelen)) NL else if (ns)\
              cp+=file.WriteBinary(" ",1);if (!cp) {if (ctld)\
              file.WriteFormatted("\x4%c",ctld); WRTPFX; } file.WriteBinary(ss1,l2);\
              cp+=l3;ss1=nullptr;l2=l3=0;ns=1;}}

static int brtnm;

static int quotes_nrm_l;
static int quotes_ind_l;

using std::string;
using std::unique_ptr;
using namespace wwiv::sdk;
using namespace wwiv::strings;

static bool ste(int i) {
  if (irt_name[i] == 32 && irt_name[i + 1] == 'O' && irt_name[i + 2] == 'F' && irt_name[i + 3] == 32) {
    if (irt_name[ i + 4 ] > 47 && irt_name[ i + 4 ] < 58) {
      return false;
    }
  }
  if (irt_name[i] == 96) {
    brtnm++;
  }
  return true;
}


char *GetQuoteInitials() {
  static char s_szQuoteInitials[8];

  brtnm = 0;
  if (irt_name[0] == 96) {
    s_szQuoteInitials[0] = irt_name[2];
  } else if (irt_name[0] == 34) {
    s_szQuoteInitials[0] = (irt_name[1] == 96) ? irt_name[3] : irt_name[1];
  } else {
    s_szQuoteInitials[0] = irt_name[0];
  }

  int i1 = 1;
  for (int i = 1; i < wwiv::strings::GetStringLength(irt_name) && i1 < 6 && irt_name[i] != '#' && irt_name[i] != '<'
       && ste(i) && brtnm != 2; i++) {
    if (irt_name[i] == 32 && irt_name[i + 1] != '#' && irt_name[i + 1] != 96 && irt_name[i + 1] != '<') {
      if (irt_name[ i + 1 ] == '(') {
        if (!isdigit(irt_name[ i + 2 ])) {
          i1 = 0;
        }
        i++;
      }
      if (irt_name[i] != '(' || !isdigit(irt_name[i + 1])) {
        s_szQuoteInitials[ i1++ ] = irt_name[ i + 1 ];
      }
    }
  }
  s_szQuoteInitials[ i1 ] = 0;
  return s_szQuoteInitials;
}

void grab_quotes(messagerec* m, const char *aux) {
  char *ss1, temp[255];
  long l2, l3;
  char *pfx;
  int cp = 0, ctla = 0, ctlc = 0, ns = 0, ctld = 0;
  int pfxlen;
  char cc = QUOTECOL + 48;
  int linelen = LINELEN, tf = 0;

  string quotes_txt_fn = StrCat(a()->temp_directory(), QUOTES_TXT);
  string quotes_ind_fn = StrCat(a()->temp_directory(), QUOTES_IND);

  File::SetFilePermissions(quotes_txt_fn, File::permReadWrite);
  File::Remove(quotes_txt_fn);
  File::SetFilePermissions(quotes_ind_fn, File::permReadWrite);
  File::Remove(quotes_ind_fn);

  if (quotes_ind) {
    free(quotes_ind);
  }

  quotes_ind = nullptr;
  quotes_nrm_l = quotes_ind_l = 0;

  if (m && aux) {
    pfx = GetQuoteInitials();
    strcat(pfx, "> ");
    pfxlen = strlen(pfx);

    string ss;
    if (readfile(m, aux, &ss)) {
      quotes_nrm_l = ss.length();
      if (ss.back() == CZ) {
        // Since CZ isn't special on Win32/Linux. Don't write it out
        // to the quotea file.
        ss.pop_back();
      }

      File quotesTextFile(quotes_txt_fn);
      if (quotesTextFile.Open(File::modeDefault | File::modeCreateFile | File::modeTruncate, File::shareDenyNone)) {
        quotesTextFile.Write(ss);
        quotesTextFile.Close();
      }
      TextFile file(quotes_ind_fn, "wb");
      if (file.IsOpen()) {
        l3 = l2 = 0;
        ss1 = nullptr;
        a()->internetFullEmailAddress = "";
        if (a()->current_net().type == network_type_t::internet) {
          for (size_t l1 = 0; l1 < ss.length(); l1++) {
            if ((ss[l1] == 4) && (ss[l1 + 1] == '0') && (ss[l1 + 2] == 'R') &&
                (ss[l1 + 3] == 'M')) {
              l1 += 3;
              while ((ss[l1] != '\r') && (l1 < ss.length())) {
                temp[l3++] = ss[l1];
                l1++;
              }
              temp[l3] = 0;
              if (strncasecmp(temp, "Message-ID", 10) == 0) {
                if (temp[0] != 0) {
                  ss1 = strtok(temp, ":");
                  if (ss1) {
                    ss1 = strtok(nullptr, "\r\n");
                  }
                  if (ss1) {
                    a()->usenetReferencesLine = ss1;
                  }
                }
              }
              l1 = ss.length();
            }
          }
        }
        l3 = l2 = 0;
        ss1 = nullptr;
        for (size_t l1 = 0; l1 < ss.length(); l1++) {
          if (ctld == -1) {
            ctld = ss[l1];
          } else switch (ss[l1]) {
            case 1:
              ctla = 1;
              break;
            case 2:
              break;
            case 3:
              if (!ss1) {
                ss1 = &ss[0] + l1;
              }
              l2++;
              ctlc = 1;
              break;
            case 4:
              ctld = -1;
              break;
            case '\n':
              tf = 0;
              if (ctla) {
                ctla = 0;
              } else {
                cc = QUOTECOL + 48;
                FLSH;
                ctld = 0;
                NL;
              }
              break;
            case ' ':
            case '\r':
              if (ss1) {
                FLSH;
              } else {
                if (ss[l1] == ' ') {
                  if (cp + 1 >= linelen) {
                    NL;
                  }
                  if (!cp) {
                    if (ctld) {
                      file.WriteFormatted("\x04%c", ctld);
                    }
                    WRTPFX;
                  }
                  cp++;
                  file.WriteBinary(" ", 1);
                }
              }
              break;
            default:
              if (!ss1) {
                ss1 = &ss[0] + l1;
              }
              l2++;
              if (ctlc) {
                if (ss[l1] == 48) {
                  ss[l1] = QUOTECOL + 48;
                }
                cc = ss[l1];
                ctlc = 0;
              } else {
                l3++;
                if (!tf) {
                  if (ss[l1] == '>') {
                    tf = 1;
                    linelen = LINELEN;
                  } else {
                    tf = 2;
                    linelen = LINELEN - 5;
                  }
                }
              }
              break;
            }
        }
        FLSH;
        if (cp) {
          file.WriteBinary("\r\n", 2);
        }
        file.Close();

        File ff(quotes_ind_fn);
        if (ff.Open(File::modeBinary | File::modeReadOnly)) {
          quotes_ind_l = ff.length();
          quotes_ind = static_cast<char*>(BbsAllocA(quotes_ind_l));
          if (quotes_ind) {
            ff.Read(quotes_ind, quotes_ind_l);
          } else {
            quotes_ind_l = 0;
          }
          ff.Close();
        }

      }
    }
  }
}

static string CreateDateString(time_t t) {
  auto dt = DateTime::from_time_t(t);
  std::ostringstream ss;
  ss << dt.to_string("%A,%B %d, %Y") << " at ";
  if (a()->user()->IsUse24HourClock()) {
    ss << dt.to_string("%H:%M");
  }
  else {
    ss << dt.to_string("%I:%M %p");
  }
  return ss.str();
}

void auto_quote(char *org, long len, int type, time_t tDateTime) {
  char s1[81], s2[81], buf[255],
       *p, *b,
       b1[81],
       *tb1;


  p = b = org;
  File fileInputMsg(a()->temp_directory(), INPUT_MSG);
  fileInputMsg.Delete();
  if (!hangup) {
    fileInputMsg.Open(File::modeBinary | File::modeCreateFile | File::modeReadWrite);
    fileInputMsg.Seek(0L, File::Whence::end);
    while (*p != '\r') {
      ++p;
    }
    *p = '\0';
    strcpy(s1, b);
    p += 2;
    len = len - (p - b);
    b = p;
    while (*p != '\r') {
      ++p;
    }
    p += 2;
    len = len - (p - b);
    b = p;
    const string datetime = CreateDateString(tDateTime);
    strcpy(s2, datetime.c_str());

    //    s2[strlen(s2)-1]='\0';
    auto tb = properize(strip_to_node(s1));
    tb1 = GetQuoteInitials();
    switch (type) {
    case 1:
      sprintf(buf, "\003""3On \003""1%s, \003""2%s\003""3 wrote:\003""0", s2, tb.c_str());
      break;
    case 2:
      sprintf(buf, "\003""3In your e-mail of \003""2%s\003""3, you wrote:\003""0", s2);
      break;
    case 3:
      sprintf(buf, "\003""3In a message posted \003""2%s\003""3, you wrote:\003""0", s2);
      break;
    case 4:
      sprintf(buf, "\003""3Message forwarded from \003""2%s\003""3, sent on %s.\003""0",
              tb.c_str(), s2);
      break;
    }
    strcat(buf, "\r\n");
    fileInputMsg.Writeln(buf, strlen(buf));
    while (len > 0) {
      while ((strchr("\r\001", *p) == nullptr) && ((p - b) < (len < 253 ? len : 253))) {
        ++p;
      }
      if (*p == '\001') {
        *(p++) = '\0';
      }
      *p = '\0';
      if (*b != '\004' && strchr(b, '\033') == nullptr) {
        int jj = 0;
        for (int j = 0; j < static_cast<int>(77 - (strlen(tb1))); j++) {
          if (((b[j] == '0') && (b[j - 1] != '\003')) || (b[j] != '0')) {
            b1[jj] = b[j];
          } else {
            b1[jj] = '5';
          }
          b1[jj + 1] = 0;
          jj++;
        }
        sprintf(buf, "\003""1%s\003""7>\003""5%s\003""0", tb1, b1);
        fileInputMsg.Writeln(buf, strlen(buf));
      }
      p += 2;
      len = len - (p - b);
      b = p;
    }
    fileInputMsg.Close();
    if (a()->user()->GetNumMessagesPosted() < 10) {
      printfile(QUOTE_NOEXT);
    }
    irt_name[0] = '\0';
  }
}

void get_quote(int fsed) {
  static char s[141], s1[10];
  static int i, i1, i2, i3, rl;
  static int l1, l2;

  if (quotes_ind == nullptr) {
    if (fsed) {
      bout << "\x0c";
    } else {
      bout.nl();
    }
    bout << "Not replying to a message!  Nothing to quote!\r\n\n";
    if (fsed) {
      pausescr();
    }
    return;
  }
  rl = 1;
  do {
    if (fsed) {
      bout << "\x0c";
    }
    if (rl) {
      i = 1;
      l1 = l2 = 0;
      i1 = i2 = 0;
      bool abort = false;
      bool next = false;
      do {
        if (quotes_ind[l2++] == 10) {
          l1++;
        }
      } while ((l2 < quotes_ind_l) && (l1 < 2));
      do {
        if (quotes_ind[l2] == 0x04) {
          while ((quotes_ind[l2++] != 10) && (l2 < quotes_ind_l)) {
          }
        } else {
          if (irt_name[0]) {
            s[0] = 32;
            i3 = 1;
          } else {
            i3 = 0;
          }
          if (abort) {
            do {
              l2++;
            } while (quotes_ind[l2] != RETURN && l2 < quotes_ind_l);
          } else {
            do {
              s[i3++] = quotes_ind[l2++];
            } while (quotes_ind[l2] != RETURN && l2 < quotes_ind_l);
          }
          if (quotes_ind[l2]) {
            l2 += 2;
            s[i3] = 0;
          }
          sprintf(s1, "%3d", i++);
          bout.bputs(s1, &abort, &next);
          bout.bpla(s, &abort);
        }
      } while (l2 < quotes_ind_l);
      --i;
    }
    bout.nl();

    if (!i1 && !hangup) {
      do {
        sprintf(s, "Quote from line 1-%d? (?=relist, Q=quit) ", i);
        bout << "|#2" << s;
        input(s, 3);
      } while (!s[0] && !hangup);
      if (s[0] == 'Q') {
        rl = 0;
      } else if (s[0] != '?') {
        i1 = atoi(s);
        if (i1 >= i) {
          i2 = i1 = i;
        }
        if (i1 < 1) {
          i1 = 1;
        }
      }
    }

    if (i1 && !i2 && !hangup) {
      do {
        bout << "|#2through line " << i1 << "-" << i << "? (Q=quit) ";
        input(s, 3);
      } while (!s[0] && !hangup);
      if (s[0] == 'Q') {
        rl = 0;
      } else if (s[0] != '?') {
        i2 = atoi(s);
        if (i2 > i) {
          i2 = i;
        }
        if (i2 < i1) {
          i2 = i1;
        }
      }
    }
    if (i2 && rl && !hangup) {
      if (i1 == i2) {
        bout << "|#5Quote line " << i1 << "? ";
      } else {
        bout << "|#5Quote lines " << i1 << "-" << i2 << "? ";
      }
      if (!noyes()) {
        i1 = 0;
        i2 = 0;
      }
    }
  } while (!hangup && rl && !i2);
  charbufferpointer = 0;
  if (i1 > 0 && i2 >= i1 && i2 <= i && rl && !hangup) {
    bquote = i1;
    equote = i2;
  }
}
