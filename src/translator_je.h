/******************************************************************************
 *
 *
 *
 * Copyright (C) 1997-2015 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby
 * granted. No representations are made about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

/*
 * translator_jp.h
 *
 * 1.2.5)
 * First Translation
 *      by Kenji Nagamatsu
 * 1.2.12)
 * Update and Shift-Jis(_WIN32)
 *      by Ryunosuke Sato (30-Dec-2001)
 */

#ifndef TRANSLATOR_JE_H
#define TRANSLATOR_JE_H

class TranslatorJapaneseEn : public TranslatorEnglish
{
  public:
    virtual QCString idLanguage()
    { return "japanese-en"; }
    virtual QCString latexLanguageSupportCommand()
    {
      return "\\usepackage{CJKutf8}\n";
    }
    virtual QCString latexFontenc()
    {
      return "";
    }
    virtual QCString latexDocumentPre()
    {
      return "\\begin{CJK}{UTF8}{min}\n";
    }
    virtual QCString latexDocumentPost()
    {
      return "\\end{CJK}\n";
    }
    virtual QCString trRTFansicp()
    {
      return "932";
    }

    /*! Used as ansicpg for RTF fcharset
     *  \see trRTFansicp() for a table of possible values.
     */
    virtual QCString trRTFCharSet()
    {
      return "128";
    }

    virtual QCString trISOLang()
    {
      return "ja";
    }
    virtual QCString getLanguageString()
    {
      return "0x411 Japanese";
    }
};

#endif
