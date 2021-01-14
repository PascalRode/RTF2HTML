// RTF2HTMLConverter.cpp: implementation of the CRTF_HTMLConverter class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RTF2HTML.h"
#include "RTF2HTMLTree.h"
#include "RTF2HTMLConverter.h"
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRTF_HTMLConverter::CRTF_HTMLConverter(TMode enMode)
{
    ASSERT(enMode == c_modRTF2HTML); //vice versa unsupported at the moment
    m_enMode = enMode;
    m_RTFTree = new CRTFTree;
    m_strTitle = "RTF2HTML Generated Document";
    ResetMetaData();
}

CRTF_HTMLConverter::~CRTF_HTMLConverter()
{
    delete m_RTFTree;
}

void CRTF_HTMLConverter::SetTitle(const CString& strTitle)
{
    m_strTitle = strTitle;
}

void CRTF_HTMLConverter::ResetMetaData()
{
    m_strCharset = "1252";
    m_strActFontColor = "#000000";
    m_strActFontName = "Arial";
    m_strActFontSize = "10";
    m_mapFontNames.RemoveAll();
    m_arrColors.RemoveAll();
    for (int i = 0; i < m_arrHTMLElements.GetSize(); i++) delete m_arrHTMLElements[i];
    m_arrHTMLElements.RemoveAll();
}

void CRTF_HTMLConverter::ConvertRTF2HTML()
{
    //Initializing
    m_strHTML = "";

    //Check_Valid_RTF
    m_strRTF.TrimRight("\n");
    m_strRTF.TrimRight("\r");
    CString strEndChar = m_strRTF.Right(1);
    strEndChar;
    if (!((m_strRTF.GetLength() >= 7) && (m_strRTF.Left(6) == "{\\rtf1") && (m_strRTF.Right(1) == "}"))) {

        fprintf(stdout, "Invalid RTF file. Must start with {RTF1 and end with } \n");
        //Invalid RTF file. Must start with "{RTF1" and end with "}"
        ASSERT(FALSE);

        return;
    }

    //Clear internal members
    ResetMetaData();

    //Build up RTF Tree. Each tree in a node coresponds to a {} section in the RTF file (code)
    //and has a designator (name), like \RTF or \FONTTBL. We start with the whole RTF file
    //(Section \RTF1)  
    delete m_RTFTree;
    m_RTFTree = new CRTFTree;
    CRTFNode NodeRoot = R2H_BuildTree(m_strRTF);
    CString strRTFCode = (NodeRoot)->m_strThisCode;

    //Fill internal meta data members
    R2H_SetMetaData(NodeRoot);

    //HTML Header
    //m_strHTML += R2H_GetHTMLHeader();

    //Create html main body 
    R2H_CreateHTMLElements(strRTFCode);
    CString strHTMLText;

    //Get HTML main body as string
    R2H_GetHTMLElements(strHTMLText);
    m_strHTML += "    " + strHTMLText + "\r\n";

    //HTML Footer
    // m_strHTML += R2H_GetHTMLFooter();
}

CString CRTF_HTMLConverter::R2H_GetHTMLHeader()
{
    CString strHTMLHeader;
    // strHTMLHeader+="<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//DE\" \"http://www.w3.org/TR/html4/loose.dtd\">\r\n";
    strHTMLHeader += "<!DOCTYPE HTML>\r\n";

    strHTMLHeader += "<html>\r\n";
    strHTMLHeader += "  <head>\r\n";
    // strHTMLHeader+="     <meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-"+m_strCharset+"\">\r\n";
    // strHTMLHeader+="     <title>"+m_strTitle+"</title>\r\n";
    strHTMLHeader += "  </head>\r\n";
    strHTMLHeader += "  <body>\r\n";
    return strHTMLHeader;
}

CString CRTF_HTMLConverter::R2H_GetHTMLFooter()
{
    CString strHTMLFooter;
    strHTMLFooter += "  </body>\r\n";
    strHTMLFooter += "</html>\r\n";
    return strHTMLFooter;
}

CString CRTF_HTMLConverter::R2H_GetRTFTag(const CString& strRTFSource, long lPos)
{
    //Initializing
    CString strTag;
    ASSERT(lPos < strRTFSource.GetLength()); //lPos is POS(\) within strRTFSource
    ASSERT(strRTFSource.GetAt(lPos) == '\\');
    int nCnt = 0;

    //Starting from lPos, we say everything is an rtf element until \ or blank or { occours
    for (int iStrPos = lPos; iStrPos < strRTFSource.GetLength(); iStrPos++) {
        if (iStrPos == lPos) continue; //(we do not want the backslash (\) itself
        CString strChTest = strRTFSource[iStrPos];
        if (strChTest.SpanIncluding(" {}\\\r") != "") break; //designator terminated    
        if ((strTag.GetLength() >= 3) && (strTag.GetAt(0) == '\'')) break;
        strTag += strChTest;
        nCnt++;
    }

    return strTag;
}

void CRTF_HTMLConverter::R2H_SetMetaData(CRTFNode& NodeRoot)
{
    //Initializing
    CString strRTFSource = (NodeRoot)->m_strThisCode;

    //Go thru RTF main string's global data
    for (int iStrPos = 0; iStrPos < strRTFSource.GetLength(); iStrPos++) {
        CString strChTest = strRTFSource[iStrPos];
        if (strChTest == "\\") {
            CString strTag = R2H_GetRTFTag(strRTFSource, iStrPos);
            CString strTestTag;

            strTestTag = "ansicpg";
            if ((strTag.GetLength() >= strTestTag.GetLength()) && (strTag.Left(strTestTag.GetLength()) == strTestTag)) {
                m_strCharset = strTag;
                m_strCharset.Delete(0, strTestTag.GetLength());
            }

            iStrPos += strTag.GetLength();
            if (((iStrPos + 1) < strRTFSource.GetLength()) && (strRTFSource[iStrPos + 1] == ' ')) iStrPos++; //Ignore Blank after Tag
        }
        else {
            //Normal character
        }
    } //loop thru string   

    //Go thru 1st-level-nodes
    for (int iRTFNodes = 0; iRTFNodes < NodeRoot.Count; iRTFNodes++) {
        //Color table
        if (NodeRoot.Nodes[iRTFNodes]->m_strName == "colortbl") {
            CString strColorTbl = NodeRoot.Nodes[iRTFNodes]->m_strThisCode;
            strColorTbl.Delete(0, NodeRoot.Nodes[iRTFNodes]->m_strName.GetLength() + 1); //+1=leading \ (backslash)
            //Palette entries are separated with semicolon ;
            CStringArray arrStrColors;
            StringToArray(strColorTbl, arrStrColors);
            for (int iColor = 0; iColor < arrStrColors.GetSize(); iColor++) {
                //RGB values within palette entry are \redXXX\greenXXX\blueXXX
                DWORD r = 0, g = 0, b = 0;
                CString strColorEntry = arrStrColors[iColor];
                strColorEntry.TrimLeft();
                strColorEntry.TrimRight();
                if (strColorEntry.GetLength() > 0) {
                    //Loop thru base colors (r,g,b) and set them
                    CStringArray arrStrRGBs;
                    R2H_GetRTFTags(strColorEntry, arrStrRGBs);
                    ASSERT(arrStrRGBs.GetSize() == 3); //RTF must contain exactly 3 entries for red/green/blue
                    for (int iBaseColor = 0; iBaseColor < arrStrRGBs.GetSize(); iBaseColor++) {
                        CString strBaseColor = arrStrRGBs[iBaseColor];
                        CString strTestBaseColor;
                        strTestBaseColor = "red";
                        if ((strBaseColor.GetLength() >= strTestBaseColor.GetLength()) && (strBaseColor.Left(strTestBaseColor.GetLength()) == strTestBaseColor)) {
                            strBaseColor.Delete(0, strTestBaseColor.GetLength());
                            r = StringToLong(strBaseColor);
                        }
                        strTestBaseColor = "green";
                        if ((strBaseColor.GetLength() >= strTestBaseColor.GetLength()) && (strBaseColor.Left(strTestBaseColor.GetLength()) == strTestBaseColor)) {
                            strBaseColor.Delete(0, strTestBaseColor.GetLength());
                            g = StringToLong(strBaseColor);
                        }
                        strTestBaseColor = "blue";
                        if ((strBaseColor.GetLength() >= strTestBaseColor.GetLength()) && (strBaseColor.Left(strTestBaseColor.GetLength()) == strTestBaseColor)) {
                            strBaseColor.Delete(0, strTestBaseColor.GetLength());
                            b = StringToLong(strBaseColor);
                        }
                    }
                }
                COLORREF ref = RGB(r, g, b);
                m_arrColors.Add(ref);
            }
            continue;
        }
        //Font Table
        if (NodeRoot.Nodes[iRTFNodes]->m_strName == "fonttbl") {
            for (int iFontNodes = 0; iFontNodes < NodeRoot.Nodes[iRTFNodes].Count; iFontNodes++) {
                CString strFontName = NodeRoot.Nodes[iRTFNodes].Nodes[iFontNodes]->m_strName;
                CString strFont = NodeRoot.Nodes[iRTFNodes].Nodes[iFontNodes]->m_strPlain;
                ASSERT((strFontName.GetLength() >= 2) && (strFontName.Left(1) == "f")); //No valid RTF
                ASSERT(strFont.GetLength() > 0);
                strFont.TrimLeft();
                strFont.TrimRight();
                strFont.Remove(';');
                m_mapFontNames.SetAt(strFontName, strFont);
            } //for-each-font
            continue;
        } //if font-table
    } //for each 1st-level-node
}

void CRTF_HTMLConverter::R2H_InterpretTag(const CString& strRTFTag)
{
    CString strTestTag;
    CHTMLElement* pElement = new CHTMLElement();

    //Char attributes
    if (strRTFTag == "b") {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "b";
    }
    else if (strRTFTag == "b0") {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLEnd;
        pElement->m_strNodeText = "b";
    }
    else if (strRTFTag == "i") {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "i";
    }
    else if (strRTFTag == "i0") {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLEnd;
        pElement->m_strNodeText = "i";
    }
    else if (strRTFTag == "ul") {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "u";
    }
    else if (strRTFTag == "ulnone") {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLEnd;
        pElement->m_strNodeText = "u";
    }

    else if (strRTFTag == "tab") {
        pElement->m_enNodeType = CHTMLElement::c_nodText;
        pElement->m_strNodeText = "&nbsp;&nbsp;&nbsp;";
    }

    //Special character (umlaut)
    strTestTag = "'";
    if ((pElement->m_enNodeType == CHTMLElement::c_nodInvalid) && (strRTFTag.GetLength() >= strTestTag.GetLength()) && (strRTFTag.Left(strTestTag.GetLength()) == strTestTag)) {
        CString strSpecialChar = strRTFTag;
        strSpecialChar.Delete(0, strTestTag.GetLength());
        ASSERT(strSpecialChar.GetLength() == 2); //Invalid RTF (must be 2-digit hex code)
        strSpecialChar = strSpecialChar.Left(2);
        pElement->m_enNodeType = CHTMLElement::c_nodText;
        pElement->m_strNodeText = "&#x" + strSpecialChar + ";";
    }

    //Paragraph Tag
    if ((pElement->m_enNodeType == CHTMLElement::c_nodInvalid) && (strRTFTag == "par")) {
        long lLastUnclosedPStart = -1;
        //Look if we first must close paragraph
        for (int iLastElements = m_arrHTMLElements.GetSize() - 1; iLastElements >= 0; iLastElements--) {
            CHTMLElement* pElementTest = m_arrHTMLElements[iLastElements];
            if ((pElementTest->m_enNodeType == CHTMLElement::c_nodHTMLEnd) && (pElementTest->m_strNodeText == "div")) break; //everything is OK
            if ((pElementTest->m_enNodeType == CHTMLElement::c_nodHTMLBegin) && (pElementTest->m_strNodeText == "div")) {
                lLastUnclosedPStart = iLastElements;
                break; //everything is OK
            }
        }
        if (lLastUnclosedPStart >= 0) {
            //Look if there is no text between last <p> and this <p-end> (e.g. <p></p>)
            //HTML does then not display a linefeed, therefore make it to <p>&nbsp;</p>
            BOOL bLastParaEmpty = TRUE;
            for (int iLastPara = lLastUnclosedPStart; iLastPara < m_arrHTMLElements.GetSize(); iLastPara++) {
                CHTMLElement* pElementTest = m_arrHTMLElements[iLastPara];
                if (pElementTest->m_enNodeType == CHTMLElement::c_nodText) {
                    if ((pElementTest->m_strNodeText != "") &&
                        (pElementTest->m_strNodeText != "\r") &&
                        (pElementTest->m_strNodeText != "\n") &&
                        (pElementTest->m_strNodeText != "\r\n") &&
                        (pElementTest->m_strNodeText != "b"))
                    {
                        bLastParaEmpty = FALSE;
                    }
                }
            }
            if (bLastParaEmpty) { //Insert modified blank (see above)
                CHTMLElement* pElementBlank = new CHTMLElement();
                pElementBlank->m_enNodeType = CHTMLElement::c_nodText;
                pElementBlank->m_strNodeText = "&nbsp;<br>";
                m_arrHTMLElements.Add(pElementBlank);
            }
            //Insert Closing </p>
            CHTMLElement* pElementClose = new CHTMLElement();
            pElementClose->m_enNodeType = CHTMLElement::c_nodHTMLEnd;
            pElementClose->m_strNodeText = "div";
            m_arrHTMLElements.Add(pElementClose);
        }
        //Add paragraph tag (<p>
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "div";

       
    }
    else {
        ; //Unknown RTF tag, just ignore
    }

    //Paragraph Alignment
    strTestTag = "q";
    if ((pElement->m_enNodeType == CHTMLElement::c_nodInvalid) && (strRTFTag.GetLength() >= strTestTag.GetLength()) && (strRTFTag.Left(strTestTag.GetLength()) == strTestTag)) {
        //Get RTF alignment
        CString strAlignRTF, strAlignHTML;
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "font";
        strAlignRTF = strRTFTag;
        strAlignRTF.Delete(0, strTestTag.GetLength());
        ASSERT(strAlignRTF.GetLength() == 1); //Invalid RTF
        //Convert RTF options to HTML options
        if (strAlignRTF == "l") {
            strAlignHTML = "left";
        }
        else if (strAlignRTF == "r") {
            strAlignHTML = "right";
        }
        else if (strAlignRTF == "c") {
            strAlignHTML = "center";
        }
        else {
            ; //unsupported
        }
        //Find last paragraph
        long lLastParaStart = -1;
        for (int iLastElements = m_arrHTMLElements.GetSize() - 1; iLastElements >= 0; iLastElements--) {
            CHTMLElement* pElementTest = m_arrHTMLElements[iLastElements];
            if ((pElementTest->m_enNodeType == CHTMLElement::c_nodHTMLBegin) && (pElementTest->m_strNodeText == "div")) {
                lLastParaStart = iLastElements;
                break; //everything is OK
            }
        }
        if ((lLastParaStart >= 0) && (strAlignHTML != "")) {
            CHTMLElement* pElementPara = m_arrHTMLElements[lLastParaStart];
            pElementPara->m_mapParams.SetAt("align", "\"" + strAlignHTML + "\"");
        }

    }

    //font color
    strTestTag = "cf";
    if ((pElement->m_enNodeType == CHTMLElement::c_nodInvalid) && (strRTFTag.GetLength() >= strTestTag.GetLength()) && (strRTFTag.Left(strTestTag.GetLength()) == strTestTag)) {
        CString strActColor;
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "font";
        strActColor = strRTFTag;
        strActColor.Delete(0, strTestTag.GetLength());
        ASSERT(strActColor.GetLength() > 0); //Invalid RTF
        long lActColor = StringToLong(strActColor);
        ASSERT(lActColor < m_arrColors.GetSize()); //Color not in Colortable !
        if (lActColor < m_arrColors.GetSize()) {
            DWORD r = 0, g = 0, b = 0;
            COLORREF ref = m_arrColors[lActColor];
            r = GetRValue(ref);
            g = GetGValue(ref);
            b = GetBValue(ref);
            CString strHTMLColor;
            strHTMLColor.Format("#%02x%02x%02x", r, g, b);
            m_strActFontColor = strHTMLColor;
        }
    }
    //font size
    strTestTag = "fs";
    if ((pElement->m_enNodeType == CHTMLElement::c_nodInvalid) && (strRTFTag.GetLength() >= strTestTag.GetLength()) && (strRTFTag.Left(strTestTag.GetLength()) == strTestTag)) {
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "font";
        m_strActFontSize = strRTFTag;
        m_strActFontSize.Delete(0, strTestTag.GetLength());
        ASSERT(m_strActFontSize.GetLength() > 0); //Invalid RTF
        m_strActFontSize = LongToString(StringToLong(m_strActFontSize) / 2); //RTF stores the doubled font size
    }
    //font name
    strTestTag = "f";  //f+number
    if ((pElement->m_enNodeType == CHTMLElement::c_nodInvalid) && (strRTFTag.GetLength() >= strTestTag.GetLength()) && (strRTFTag.Left(strTestTag.GetLength()) == strTestTag) && (strRTFTag.Mid(1).SpanIncluding("01234567890") == strRTFTag.Mid(1))) {
        CString strActFontDsgn = strRTFTag;
        pElement->m_enNodeType = CHTMLElement::c_nodHTMLBegin;
        pElement->m_strNodeText = "font";
        ASSERT(strActFontDsgn.GetLength() > 0); //Invalid RTF
        CString strActFontName;
        BOOL bFound = m_mapFontNames.Lookup(strActFontDsgn, strActFontName);
        ASSERT(bFound); //Font not found in font table, don't change font
        if (bFound) m_strActFontName = strActFontName;
    }

    //New font tag ?
    if ((pElement->m_enNodeType == CHTMLElement::c_nodHTMLBegin) && (pElement->m_strNodeText == "font")) {
        BOOL bMustClose = FALSE;
        //Look if we first must close paragraph
        for (int iLastElements = m_arrHTMLElements.GetSize() - 1; iLastElements >= 0; iLastElements--) {
            CHTMLElement* pElementTest = m_arrHTMLElements[iLastElements];
            if ((pElementTest->m_enNodeType == CHTMLElement::c_nodHTMLEnd) && (pElementTest->m_strNodeText == "font")) break; //everything is OK
            if ((pElementTest->m_enNodeType == CHTMLElement::c_nodHTMLBegin) && (pElementTest->m_strNodeText == "font")) {
                bMustClose = TRUE;
                break; //everything is OK
            }
        }
        if (bMustClose) {
            //Insert Closing </p>
            CHTMLElement* pElementClose = new CHTMLElement();
            pElementClose->m_enNodeType = CHTMLElement::c_nodHTMLEnd;
            pElementClose->m_strNodeText = "font";
            m_arrHTMLElements.Add(pElementClose);
        }
        //Set font tag options
        pElement->m_mapParams.SetAt("color", "\"" + m_strActFontColor + "\"");
        pElement->m_mapParams.SetAt("style", "\"font-size: " + m_strActFontSize + "pt;font-family:" + m_strActFontName + ";\"");
    }
    if (pElement->m_enNodeType != CHTMLElement::c_nodInvalid) m_arrHTMLElements.Add(pElement);
}

void CRTF_HTMLConverter::R2H_GetRTFTags(const CString& strRTFSource, CStringArray& arrTgt)
{
    //Go thru RTF main string
    for (int iStrPos = 0; iStrPos < strRTFSource.GetLength(); iStrPos++) {
        CString strChTest = strRTFSource[iStrPos];
        if (strChTest == "\\") {
            CString strTag = R2H_GetRTFTag(strRTFSource, iStrPos);
            arrTgt.Add(strTag);
            iStrPos += strTag.GetLength();
            if (((iStrPos + 1) < strRTFSource.GetLength()) && (strRTFSource[iStrPos + 1] == ' ')) iStrPos++; //Ignore Blank after Tag
        }
    } //loop thru string
}


//! Gets the created HTML elements as HTML text
void CRTF_HTMLConverter::R2H_GetHTMLElements(CString& strHTML)
{
    strHTML = "";

    //Loop thru all HTML elements
    for (int iElem = 0; iElem < m_arrHTMLElements.GetSize(); iElem++) {
        CHTMLElement* pElem = m_arrHTMLElements[iElem];
        if (pElem->m_enNodeType == CHTMLElement::c_nodHTMLBegin) {
            //HTML element open tag
            strHTML += "<";
            strHTML += pElem->m_strNodeText;
            //HML element parameters (<font param1="test" param2="hugo">
            POSITION pos;
            pos = pElem->m_mapParams.GetStartPosition();
            while (pos) {
                CString strKey, strValue;
                pElem->m_mapParams.GetNextAssoc(pos, strKey, strValue);
                strHTML += " " + strKey + " = " + strValue;
            }
            strHTML += ">";
        }
        else if (pElem->m_enNodeType == CHTMLElement::c_nodHTMLEnd) {
            strHTML += "<";
            strHTML += "/";
            strHTML += pElem->m_strNodeText;
            strHTML += ">";
        }
        else if (pElem->m_enNodeType == CHTMLElement::c_nodText) {
            strHTML += pElem->m_strNodeText;
        }
        else {
            ASSERT(FALSE); //internal error (wrong html tag)
        }

    }
}

void CRTF_HTMLConverter::R2H_CreateHTMLElements(const CString& strRTFSource)
{
    //Go thru RTF main string
    CString strCurrentText;
    for (int iStrPos = 0; iStrPos < strRTFSource.GetLength(); iStrPos++) {
        CString strChTest = strRTFSource[iStrPos];
        if (strChTest == "\\") {
            //New tag
            if (strCurrentText != "") {

                // Replace whitespace -> otherwise ignored in HTML at the beginning               
                strCurrentText.Replace(" ", "&nbsp;"); 
                

                CHTMLElement* pElement = new CHTMLElement();
                pElement->m_enNodeType = CHTMLElement::c_nodText;
                pElement->m_strNodeText = strCurrentText;
                m_arrHTMLElements.Add(pElement);
                strCurrentText = "";
            }
            CString strTag = R2H_GetRTFTag(strRTFSource, iStrPos);
            R2H_InterpretTag(strTag);
            iStrPos += strTag.GetLength();
           if (((iStrPos + 1) < strRTFSource.GetLength()) && (strRTFSource[iStrPos + 1] == ' ')) iStrPos++; //Ignore Blank after Tag
        }
        else {
            //Normal character
            strCurrentText += strChTest;
        }
    } //loop thru string
  
    if (strCurrentText != "") {
        CHTMLElement* pElement = new CHTMLElement();
        pElement->m_enNodeType = CHTMLElement::c_nodText;
        pElement->m_strNodeText = strCurrentText;
        m_arrHTMLElements.Add(pElement);
        strCurrentText = "";
    }
}

CRTF_HTMLConverter::CRTFNode CRTF_HTMLConverter::R2H_BuildTree(const CString& strSource, CRTFNode* pNodeParent)
{
    //Initializing   
    CString strName;

    //Extract Node's Name
    if ((strSource.GetLength() > 2) && (strSource[0] == '{') && (strSource[1] == '\\')) {
        for (int iStrPos = 2; iStrPos < strSource.GetLength(); iStrPos++) {
            CString strChTest = strSource[iStrPos];
            if (strChTest.SpanIncluding(" {}\\\r") != "") {
                break;
            }
            else {
                strName += strChTest;
            }
        }
    }
    else {
        //Invalid RTF Node. RTF Nodes must start with a { and then contain a \NAME identifier
        ASSERT(FALSE);
        return CRTFNode();
    }

    //Extract pure text
    CString strNodeText = strSource;
    strNodeText.Delete(0, 1);
    strNodeText.Delete(strNodeText.GetLength() - 1, 1);

    //Add node into tree
    CRTFNodeA nodeA;
    nodeA.m_strCode = strSource;
    nodeA.m_strName = strName;
    CRTFNode nodeThis;
    if (pNodeParent) {
        nodeThis = pNodeParent->AddNode(nodeA);
    }
    else {
        nodeThis = m_RTFTree->AddNode(nodeA);
    }

    //Looking for children
    CString strNodeNew;
    CString strThisCode;
    long lLevel = 0; //# Of opened {
    for (int iStrPos = 0; iStrPos < strNodeText.GetLength(); iStrPos++) {
        CString strChTest = strNodeText[iStrPos];
        if (strChTest == "{") {
            //New element
            if (lLevel == 0) {
                strNodeNew = "";
            }
            else { ; } //Nested Element, will be added during recurse
            lLevel++;
        }
        if (lLevel > 0) {
            strNodeNew += strChTest;
        }
        else {
            strThisCode += strChTest;
        }
        if (strChTest == "}") {
            ASSERT(lLevel > 0); //Invalid RTF, more closing } than opening ´{
            lLevel--;
            if (lLevel == 0) {
                //Recurse (new Sub-Node (child) ready
                R2H_BuildTree(strNodeNew, &nodeThis);
            }
            else { ; } //Nested Element, will be added during recurse
        }
    } //loop thru string
    ASSERT(lLevel == 0); //Invalid RTF, more opening { than closing }

    //Get the plain text (without \)
    CString strPlain;
    for (int iStrPos = 0; iStrPos < strNodeText.GetLength(); iStrPos++) {
        CString strChTest = strNodeText[iStrPos];
        if (strChTest == "\\") {
            CString strTag = R2H_GetRTFTag(strNodeText, iStrPos);
            iStrPos += strTag.GetLength();
            if (((iStrPos + 1) < strNodeText.GetLength()) && (strNodeText[iStrPos + 1] == ' ')) iStrPos++; //Ignore Blank after Tag
        }
        else {
            strPlain += strChTest;
        }
    } //loop thru string

    nodeThis->m_strPlain = strPlain;
    nodeThis->m_strThisCode = strThisCode;
    return nodeThis;
}

CString& operator<< (CString& os, CRTF_HTMLConverter& conv)
{
    //For streaming operations
    if (conv.m_enMode == CRTF_HTMLConverter::c_modRTF2HTML) {
        os = conv.m_strHTML;
    }
    else {
        os = conv.m_strHTML;
    }
    return os;
}

CString& operator>> (CString& is, CRTF_HTMLConverter& conv)
{
    //For streaming operations
    CString strTemp;
    strTemp = is;
    //RTF->HTML
    if (conv.m_enMode == CRTF_HTMLConverter::c_modRTF2HTML) {
        conv.m_strRTF = strTemp;
        conv.ConvertRTF2HTML();
    }
    return is;
}

CRTF_HTMLConverter::CHTMLElement::CHTMLElement()
{
    m_enNodeType = CHTMLElement::c_nodInvalid;
}