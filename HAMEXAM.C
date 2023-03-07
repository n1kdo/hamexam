/******************************************************************************/
/* HamExam.C -- HamExam main code.                                            */
/******************************************************************************/

#define NOKANJI
#define NOMDI
#define NOSOUND
#define NOLOGERROR
#define NOPROFILER
#define NOMETAFILE
#define NOSYSTEMPARAMSINFO
#define NOWH
#define NONLS
#define NOSERVICE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#ifdef WIN32
#define huge /* huge */
#endif /* WIN32 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "HamExam.h"
#include "TextMem.h"
#include "Dib.h"

HINSTANCE hInstance;
HWND hwndMain;
HWND figureWindow;
WNDPROC oldFigureWndProc;
WNDPROC newFigureWndProc;
QuestionGroupStruct questionGroups[MAX_GROUPS];     /* the question pool groups */
QuestionStruct questions[MAX_QUESTIONS];            /* the question pool questions */
ExamQuestionsStruct examQuestions[MAX_QUESTIONS];   /* pointers to exam questions */
int numQuestions;                                   /* number of question in the pool */
int numGroups;                                      /* number of groups in the pool */
int numExamQuestions;                               /* number of questions in the exam */
int examMode = EXAM_MODE_IDLE;                      /* the exam mode */
int questionIndex;                                  /* the current question index */
BYTE huge *figureDib = NULL;                        /* the current question's graphic */
int selectedPool = 0;                               /* the index of the selected question pool */
char helpFileName[128];                             /* the path/file name of the help file */
char path[_MAX_PATH];                               /* the path of the HamExam directory */
int numWrong, numRight;
TextRegionStruct far *textRegion;

const LPSTR poolNames[5] = {"Novice", "Technician", "General", "Advanced", "Extra"};
const LPSTR poolFileNames[5] = {"ELEMNT2.DAT",
                                "ELEMNT3A.DAT",
                                "ELEMNT3B.DAT",
                                "ELEMNT4A.DAT",
                                "ELEMNT4B.DAT"};
const int passingScore[5] = {22, 19, 19, 37, 30};
const LPSTR examModeNames[4] = {"",
                                "Practice",
                                "Practice All",
                                "Test"};


LONG WINAPI _export WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LONG WINAPI _export NewFigureWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL PassFailDialogProc (HWND, WORD, WPARAM, LPARAM);
BOOL FAR PASCAL ResultsDialogProc (HWND, WORD, WPARAM, LPARAM);
BOOL FAR PASCAL AboutDialogProc (HWND, WORD, WPARAM, LPARAM);
BOOL WINAPI WrongAnswerDialogProc (HWND, WORD, WPARAM, LPARAM);

int LoadPool(LPSTR fileName);
void SetInterfaceVisibility(BOOL visibility);
void StartExam(void);
void DisplayQuestion(void);
void GetResultsText(unsigned int *, LPSTR, int);

int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    MSG         msg;
    WNDCLASS    wndclass;

    char tempString[150];
    char modulePath[_MAX_PATH];
    int i;

    //_InitEasyWin();

    hInstance = hInst;
    if (!hPrevInstance)
    {
        wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = DLGWINDOWEXTRA;
        wndclass.hInstance      = hInstance;
        wndclass.hIcon          = LoadIcon (hInstance, APPNAME);
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = APPNAME;
        RegisterClass (&wndclass);
    }

    GetModuleFileName(hInstance, modulePath, _MAX_PATH);
    i = strlen(modulePath);
    while ((i > 0) && (modulePath[i-1] != '\\'))
        i--;
    _fstrncpy(path, modulePath, i);
    path[i] = '\0';

    _fstrcpy(helpFileName, path);
    _fstrcat(helpFileName, APPNAME);
    _fstrcat(helpFileName, ".HLP");

    hwndMain = CreateDialog (hInst, APPNAME, 0, NULL);
    if (hwndMain == NULL)
    {
        MessageBox(NULL, "Could not create main dialog window...", "Problem...", MB_ICONSTOP | MB_OK);
        return(0);
    }

    textRegion = TextCreate(150000);
    randomize();

    sprintf(tempString, "%s version %s %s", APPNAME, VERSION, COPYRIGHT);
    SetWindowText(GetDlgItem(hwndMain, COPYRIGHT_TEXT), tempString);

    figureWindow = GetDlgItem(hwndMain, FIGURE_WINDOW);
    oldFigureWndProc = (WNDPROC) GetWindowLong(figureWindow, GWL_WNDPROC);
    SetWindowLong(figureWindow, GWL_WNDPROC, (LONG) NewFigureWndProc);

    ShowWindow (hwndMain, nCmdShow);

    PostMessage(hwndMain, WM_COMMAND, QUESTIONS_NOVICE + selectedPool, 0L);

    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }
    TextDestroy(textRegion);
    return msg.wParam;
}

LONG WINAPI _export WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    char tempString[120];
    QuestionStruct *question;
    int answer;

    switch (message)
    {
        case WM_COMMAND:
            switch (wParam)
            {
                case FILE_EXIT:
                    PostMessage(hwnd, WM_CLOSE, 0, 0L);
                    break;

                case EXAM_PRACTICE:
                case EXAM_PRACTICE_ALL:
                case EXAM_TEST:
                    examMode = wParam - EXAM_PRACTICE + EXAM_MODE_PRACTICE;
                    if (numQuestions)
                        StartExam();    /* don't start if no questions are loaded */
                    else
                        MessageBox(hwnd, "Can't start.\nNo questions are loaded.", "Problem...", MB_ICONEXCLAMATION | MB_OK);
                    break;

                case QUESTIONS_NOVICE:
                case QUESTIONS_TECHNICIAN:
                case QUESTIONS_GENERAL:
                case QUESTIONS_ADVANCED:
                case QUESTIONS_EXTRA:
                    selectedPool = wParam - QUESTIONS_NOVICE;
                    LoadPool(poolFileNames[selectedPool]);
                    _fstrcpy(tempString, APPNAME);
                    _fstrcat(tempString, " -- ");
                    _fstrcat(tempString, poolNames[selectedPool]);
                    _fstrcat(tempString, " Question Pool");
                    SetWindowText(hwnd, tempString);
                    break;

                case HELP_HAMEXAM:
                    WinHelp(hwnd, helpFileName, HELP_CONTENTS, 0L);
                    break;

                case HELP_ABOUT:
                    DialogBox (hInstance, "AboutDialog", hwnd, (DLGPROC) AboutDialogProc);
                    return(0);

                case ANSWER_A_BUTTON:
                case ANSWER_B_BUTTON:
                case ANSWER_C_BUTTON:
                case ANSWER_D_BUTTON:
                    answer = wParam - ANSWER_A_BUTTON;
                    question = &questions[examQuestions[questionIndex].questionIndex];
                    if (answer == question->answerNumber)
                    { /* correct answer */
                        numRight++;
                        examQuestions[questionIndex].wrongFlag = FALSE;
                    } /* if answer == question->answerNumber */
                    else
                    { /* wrong answer */
                        numWrong++;
                        examQuestions[questionIndex].wrongFlag = TRUE;
                        if (examMode != EXAM_MODE_TEST)
                        { /* inform user of wrong answer */
                            DialogBoxParam(hInstance,
                                           "WrongAnswerDialog",
                                           hwnd,
                                           (DLGPROC) WrongAnswerDialogProc,
                                           (LPARAM) MAKELONG((QuestionStruct *) question, 0));
                        } /* if examMode != EXAM_MODE_TEST */
                    } /* if answer == question->answerNumber */
                    questionIndex++;
                    if (questionIndex < numExamQuestions)
                    { /* show the next question */
                        DisplayQuestion();
                    } /* if questionIndex < numExamQuestions */
                    else
                    { /* give the results */
                        SetInterfaceVisibility(FALSE);
                        DialogBox (hInstance, "PassFailDialog", hwnd, (DLGPROC) PassFailDialogProc);
                    } /* if questionIndex < numExamQuestions */
                    return(0);

            } /* switch */
            return 0;

        case WM_CHAR:
            switch (wParam)
            {
                case 'A':
                case 'a':
                    PostMessage(hwnd, WM_COMMAND, ANSWER_A_BUTTON, 0L);
                    break;

                case 'B':
                case 'b':
                    PostMessage(hwnd, WM_COMMAND, ANSWER_B_BUTTON, 0L);
                    break;

                case 'C':
                case 'c':
                    PostMessage(hwnd, WM_COMMAND, ANSWER_C_BUTTON, 0L);
                    break;

                case 'D':
                case 'd':
                    PostMessage(hwnd, WM_COMMAND, ANSWER_D_BUTTON, 0L);
                    break;

#ifdef DEBUG_MODE
                case VK_RETURN:
                    PostMessage(hwnd, WM_COMMAND, ANSWER_A_BUTTON, 0L);
                    break;
#endif

            } /* switch wParam */
            return(0);

        case WM_CLOSE:
            SetWindowLong(figureWindow, GWL_WNDPROC, (LONG) oldFigureWndProc);
            WinHelp(hwnd, helpFileName, HELP_QUIT, 0L);
            DestroyWindow(hwnd);
            return(0);

        case WM_DESTROY:
            PostQuitMessage(0);
            return(0);
    } /* switch message */
    return DefWindowProc (hwnd, message, wParam, lParam);
}

LONG WINAPI _export NewFigureWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH paintBrush;

    switch (message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            if (figureDib != NULL)
                PaintDibInWindow(figureDib, hdc);
            else
            {
                paintBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                FillRect(hdc, &ps.rcPaint, paintBrush);
                DeleteObject(paintBrush);
            }
            EndPaint(hwnd, &ps);
            return(FALSE);
    } /* switch message */
    return(CallWindowProc(oldFigureWndProc, hwnd, message, wParam, lParam));
} /* NewFigureWndProc() */

BOOL WINAPI _export AboutDialogProc (HWND hdlg, WORD msg, WPARAM wParam, LPARAM lParam)
{
    char tempString[150];
    switch (msg)
    {
        case WM_INITDIALOG:
            sprintf(tempString, "%s version %s\n\nAn amateur radio examination training aid.\n\n%s", APPNAME, VERSION, COPYRIGHT);
            SetWindowText(GetDlgItem(hdlg, ABOUT_TEXT), tempString);


            return(TRUE);

        case WM_COMMAND:
            if (wParam == IDOK)
            {
                EndDialog (hdlg, TRUE);
                return(TRUE);
            }
            break; /* End WM_COMMAND. */

        case WM_CLOSE:
            EndDialog(hdlg, TRUE);
            return(TRUE);

    }/* switch */
    return(FALSE);
}/* AboutDialogProc */

BOOL WINAPI _export PassFailDialogProc (HWND hdlg, WORD msg, WPARAM wParam, LPARAM lParam)
{
    char passFailMessage[100];
    char scoreMessage[30];
    float pctRight;

    switch (msg)
    {
        case WM_INITDIALOG:
            if (examMode != EXAM_MODE_PRACTICE_ALL)
            {
                if (numRight >= passingScore[selectedPool])
                {
                     sprintf(passFailMessage, "Congratulations, you passed the %s exam!",
                                              poolNames[selectedPool]);
                } /* if numRight >= passingScore... */
                else
                {
                    sprintf(passFailMessage, "You failed the %s exam.", poolNames[selectedPool]);
                } /* if numRight >= passingScore... */
            } /* if examMode != EXAM_MODE_PRACTICE_ALL */
            else
            { /* exception for practice all -- grade by percentage */
                pctRight = (float) numRight / (numRight + numWrong) * (float) 100.0;
                sprintf(passFailMessage, "Your score was %5.1f%%\n", pctRight);
                if (pctRight >= 75.0)
                {
                    _fstrcat(passFailMessage, "This is a passing score.");
                } /* if pctRight >= 75 */
                else
                {
                    _fstrcat(passFailMessage, "This is a failing score.");
                } /* if pctRight >= 75 */
            }
            sprintf(scoreMessage, "\n\nYou got %d out of %d.", numRight, numRight + numWrong);
            _fstrcat(passFailMessage, scoreMessage);
            SetWindowText(GetDlgItem(hdlg, PASS_FAIL_TEXT), passFailMessage);

            return(TRUE);

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog (hdlg, TRUE);
                    return(TRUE);
                case RESULTS_BUTTON:
                    DialogBox (hInstance, "ResultsDialog", hdlg, (DLGPROC) ResultsDialogProc);
                    return(TRUE);
            } /* switch (wParam) */
            break; /* End WM_COMMAND. */

        case WM_CLOSE:
            EndDialog(hdlg, TRUE);
            return(TRUE);

    }/* switch */
    return(FALSE);
}/* PassFailDialogProc */

BOOL WINAPI _export ResultsDialogProc (HWND hdlg, WORD msg, WPARAM wParam, LPARAM lParam)
{
    LPSTR resultsText;
    unsigned int textSize;
    static int detailFlag;
    HCURSOR oldCursor;

    switch (msg)
    {
        case WM_INITDIALOG:
            oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            textSize = 0;
            detailFlag = FALSE;
            GetResultsText(&textSize, NULL, detailFlag);
            resultsText = (LPSTR) GlobalAllocPtr(GPTR, textSize + 1); /* reserve room for null */
            GetResultsText(&textSize, resultsText, detailFlag);
            SetWindowText(GetDlgItem(hdlg, RESULTS_TEXT_EDIT), resultsText);
            GlobalFreePtr(resultsText);
            SetWindowText(GetDlgItem(hdlg, SUMMARY_DETAILS_BUTTON), detailFlag ? "Summary" : "Details");
            SetWindowText(hdlg, detailFlag ? "Examination Results Detail" : "Examination Results Summary");
            SetCursor(oldCursor);
            return(TRUE);

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog(hdlg, TRUE);
                    return(TRUE);

                case SUMMARY_DETAILS_BUTTON:
                    oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    detailFlag = !detailFlag;
                    GetResultsText(&textSize, NULL, detailFlag);
                    if (textSize > 0)
                    {
                        resultsText = (LPSTR) GlobalAllocPtr(GPTR, textSize + 1);
                        GetResultsText(&textSize, resultsText, detailFlag);
                        SetWindowText(GetDlgItem(hdlg, RESULTS_TEXT_EDIT), resultsText);
                        GlobalFreePtr(resultsText);
                        SetWindowText(GetDlgItem(hdlg, SUMMARY_DETAILS_BUTTON), detailFlag ? "Summary" : "Details");
                        SetWindowText(hdlg, detailFlag ? "Examination Results Detail" : "Examination Results Summary");
                    } /* if textSize <= MAX_RESULTS_TEXT_SIZE */
                    else
                    {
                        MessageBox(hdlg,
                                   "The error list is empty.",
                                   "Information...",
                                   MB_ICONINFORMATION | MB_OK);
                    } /* if textSize <= MAX_RESULTS_TEXT_SIZE */
                    SetCursor(oldCursor);
                    return(TRUE);
            } /* switch (wParam) */
            break; /* End WM_COMMAND. */

        case WM_CLOSE:
            EndDialog(hdlg, TRUE);
            return(TRUE);

    }/* switch */
    return(FALSE);
}/* ResultsDialogProc */

BOOL WINAPI _export WrongAnswerDialogProc (HWND hdlg, WORD msg, WPARAM wParam, LPARAM lParam)
{
    QuestionStruct *question;
    char text[512];
    switch (msg)
    {
        case WM_INITDIALOG:
            question = (QuestionStruct *) LOWORD(lParam);
            _fstrcpy(text, "The answer you chose is incorrect.  The correct answer is:\n\n");
            _fstrcat(text, question->answers[question->answerNumber]);
            SetWindowText(GetDlgItem(hdlg, WRONG_ANSWER_TEXT), text);
            return(TRUE);

        case WM_COMMAND:
            if (wParam == IDOK)
            {
                EndDialog (hdlg, TRUE);
                return(TRUE);
            }
            break; /* End WM_COMMAND. */
    }/* switch */
    return(FALSE);
}/* WrongAnswerDialogProc */

int LoadPool(LPSTR fileName)
{
    int file;
    char c;
    UINT bytesRead;
    UINT byteIndex;
    LPSTR fileBuffer;
    char line[512];
    char text1[512];
    char text2[512];
    int linePos = 0;
    char filePathName[128];
    int loadPoolState = LOAD_STATE_GROUP;
    int numGroupQuestionsRead;
    int i, j;
    QuestionGroupStruct *questionGroup;
    QuestionStruct *question;
    HCURSOR oldCursor;

    oldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TextReset(textRegion);
    numQuestions = 0;
    numGroups = 0;

    _fstrcpy(filePathName, path);
    _fstrcat(filePathName, fileName);

    /* open */
    if ((file = _lopen(filePathName, OF_READ)) == -1)
    {
        sprintf(line, "Could not find file %s", filePathName);
        MessageBox(hwndMain, line, "Problem...", MB_ICONEXCLAMATION | MB_OK);
        SetCursor(oldCursor);
        return(FALSE);
    } /* if file... */

    _llseek(file, 0L, 0); /* reset to top of file */

    fileBuffer = (LPSTR) GlobalAllocPtr(GMEM_MOVEABLE, READ_BUFFER_SIZE);
    if (fileBuffer == NULL)
    {
        MessageBox(hwndMain, "Could not allocate memory for buffer!", "Problem...", MB_ICONEXCLAMATION | MB_OK);
        _lclose(file);
        SetCursor(oldCursor);
        return(FALSE);
    } /* if fileBuffer == NULL */

    /* read */
    linePos = 0;
    bytesRead = READ_BUFFER_SIZE;
    while (bytesRead == READ_BUFFER_SIZE)
    {
        bytesRead = _lread(file, (LPSTR) fileBuffer, READ_BUFFER_SIZE);
        byteIndex = 0;
        while (byteIndex < bytesRead)
        {
            c = fileBuffer[byteIndex];
            if (c == '\r')
            {
                line[linePos] = '\0';
                byteIndex++;

                switch (loadPoolState)
                {
                    case LOAD_STATE_GROUP:
                        questionGroup = &questionGroups[numGroups];
                        i = 0;
                        j = 0;
                        while (line[i] != '[')
                        {
                            text1[j++] = line[i++];
                        }
                        text1[j-1] = '\0'; /* eat the colon */
                        questionGroup->groupName = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(questionGroup->groupName, text1, j);
                        i++;
                        questionGroup->numQuestionsToAsk = 0;
                        while (line[i] != '/')
                        {
                            questionGroup->numQuestionsToAsk =
                                questionGroup->numQuestionsToAsk * 10 +
                                line[i++] - '0';
                        }
                        i++;
                        questionGroup->numQuestionsInGroup = 0;
                        while (line[i] != ']')
                        {
                            questionGroup->numQuestionsInGroup =
                                questionGroup->numQuestionsInGroup * 10 +
                                line[i++] - '0';
                        }
                        i++;
                        j = 0;
                        while (line[i])
                        {
                            text2[j++] = line[i++];
                        }
                        text2[j++] = 0;
                        questionGroup->groupTitle = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(questionGroup->groupTitle, text2, j);
                        questionGroup->firstQuestionIndex = numQuestions;
                        numGroupQuestionsRead = 0;
                        loadPoolState = LOAD_STATE_QUESTION;
                        break;

                    case LOAD_STATE_QUESTION:
                        i = 0;
                        j = 0;
                        question = &questions[numQuestions];
                        while (line[i] != '|')
                        {
                            text1[j++] = line[i++];
                        }
                        text1[j++] = '\0';
                        question->questionNumber = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(question->questionNumber, text1, j);
                        i++; /* eat the vertical bar */
                        question->answerNumber = line[i++] - 'A';
                        i++; /* eat the vertical bar */
                        j = 0;
                        while (line[i])
                        {
                            text2[j++] = line[i++];
                        }
                        text2[j++] = 0;
                        question->questionText = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(question->questionText, text2, j);
                        question->groupIndex = numGroups;
                        numGroupQuestionsRead++;
                        loadPoolState = LOAD_STATE_ANSWER_1;
                        break;

                    case LOAD_STATE_ANSWER_1:
                        j = strlen(line) + 1;
                        question->answers[0] = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(question->answers[0], line, j);
                        loadPoolState = LOAD_STATE_ANSWER_2;
                        break;

                    case LOAD_STATE_ANSWER_2:
                        j = strlen(line) + 1;
                        question->answers[1] = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(question->answers[1], line, j);
                        loadPoolState = LOAD_STATE_ANSWER_3;
                        break;

                    case LOAD_STATE_ANSWER_3:
                        j = strlen(line) + 1;
                        question->answers[2] = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(question->answers[2], line, j);
                        loadPoolState = LOAD_STATE_ANSWER_4;
                        break;

                    case LOAD_STATE_ANSWER_4:
                        j = strlen(line) + 1;
                        question->answers[3] = (LPSTR) TextMalloc(j, textRegion);
                        _fstrncpy(question->answers[3], line, j);
                        if (numGroupQuestionsRead < questionGroups[numGroups].numQuestionsInGroup)
                        {
                            numQuestions++;
                            loadPoolState = LOAD_STATE_QUESTION;
                        }
                        else
                        {
                            numQuestions++;
                            numGroups++;
                            loadPoolState = LOAD_STATE_GROUP;
                        }
                        break;

                } /* switch loadPoolState */
                linePos = 0;
            } /* if c == \r */
            else
            {
                if (c == '\n')
                {
                    byteIndex++;
                    continue;
                } /* if c  == \n*/
                else
                {
                    line[linePos++] = fileBuffer[byteIndex++];
                } /* if c  == \n*/
            } /* if c == \r */
        } /* while fileBuffer.. */
    } /* while bytesRead... */
    /* close */;
    _lclose(file);
    GlobalFreePtr(fileBuffer);
    SetCursor(oldCursor);
    return(TRUE);
} /* LoadPool() */

void SetInterfaceVisibility(BOOL visibility)
{
    int i;
    for (i = QUESTION_TEXT; i < COPYRIGHT_TEXT; i++)
        ShowWindow(GetDlgItem(hwndMain, i), visibility ? SW_SHOW : SW_HIDE);
} /* SetInterfaceVisibility() */

void StartExam()
{
    int groupCounter, questionSelector, questionTester, randomQuestion, found;

    numExamQuestions = 0;

    if (examMode == EXAM_MODE_PRACTICE_ALL)
    {
	    for (questionSelector = 0; questionSelector < numQuestions; questionSelector++)
        {
            examQuestions[questionSelector].questionIndex = questionSelector;
            examQuestions[questionSelector].wrongFlag = FALSE;
        } /* for questionSelector */
        numExamQuestions = numQuestions;
    } /* if examMode == EXAM_MODE_PRACTICE_ALL */
    else
    {
        for (groupCounter = 0; groupCounter < numGroups; groupCounter++)
        {
    	    for (questionSelector = 0; questionSelector < questionGroups[groupCounter].numQuestionsToAsk; questionSelector++)
            {
        		if (questionSelector == 0)
                {
                    examQuestions[numExamQuestions].questionIndex = random(questionGroups[groupCounter].numQuestionsInGroup) +
                                                                    questionGroups[groupCounter].firstQuestionIndex;
                    examQuestions[numExamQuestions].wrongFlag = FALSE;
        		    numExamQuestions++;
                } /* if questionSelector == 0 */
        		else
                {
        		    while (TRUE)
                    {
            			randomQuestion = random(questionGroups[groupCounter].numQuestionsInGroup) +
                                         questionGroups[groupCounter].firstQuestionIndex;
            			found = FALSE;
            			for (questionTester = 0; questionTester < numExamQuestions; questionTester++)
                        {
            			    if (examQuestions[questionTester].questionIndex == randomQuestion)
                                found = TRUE;
            			} /* for questionTester */
            			if (!found)
                        {
                            examQuestions[numExamQuestions].questionIndex = randomQuestion;
                            examQuestions[numExamQuestions].wrongFlag = FALSE;
            			    numExamQuestions++;
                            break;
            			} /* if !found */
                    } /* while TRUE */
                } /* if questionSelector == 0 */
    	    } /* for questionSelector */
    	} /* for groupCounter */
    } /* if examMode == EXAM_MODE_PRACTICE_ALL */

    /* questions are ready, preset variables */
    numWrong = 0;
    numRight = 0;
    questionIndex = 0;
    SetInterfaceVisibility(TRUE);
    DisplayQuestion();
} /* StartExam() */

void DisplayQuestion(void)
{
    LPSTR figurePtr;
    char tempString[128];
    char errorMsg[150];
    int i;
    QuestionStruct *question;

    if (figureDib != NULL)
    {
        FreeDib(&figureDib);
        InvalidateRect(figureWindow, NULL, FALSE);
    }

    question = &questions[examQuestions[questionIndex].questionIndex];
    SetWindowText(GetDlgItem(hwndMain, QUESTION_TEXT), question->questionText);
    SetWindowText(GetDlgItem(hwndMain, ANSWER_A_TEXT), question->answers[0]);
    SetWindowText(GetDlgItem(hwndMain, ANSWER_B_TEXT), question->answers[1]);
    SetWindowText(GetDlgItem(hwndMain, ANSWER_C_TEXT), question->answers[2]);
    SetWindowText(GetDlgItem(hwndMain, ANSWER_D_TEXT), question->answers[3]);
    SetWindowText(GetDlgItem(hwndMain, QUESTION_NUMBER_TEXT), question->questionNumber);
    sprintf(tempString, "%d/%d", questionIndex+1, numExamQuestions);
    SetWindowText(GetDlgItem(hwndMain, QUESTION_OF_TEXT), tempString);
    figurePtr = _fstrstr(question->questionText, FIGURE_NAME);
    if (figurePtr != NULL)
    {
        figurePtr += _fstrlen(FIGURE_NAME);
        i = 0;
        while (path[i])
            tempString[i] = path[i++];

        while ((*figurePtr) &&
               (*figurePtr != ' ') &&
               (*figurePtr != ']') &&
               (*figurePtr != ',') &&
               (*figurePtr != '?'))
            tempString[i++] = *figurePtr++;
        tempString[i++] = '.';
        tempString[i++] = 'B';
        tempString[i++] = 'M';
        tempString[i++] = 'P';
        tempString[i] = '\0';

        figureDib = ReadDib(tempString);
        if (figureDib == NULL)
        {
            sprintf(errorMsg, "Could not find file %s", tempString);
            MessageBox(hwndMain, errorMsg, "Problem...", MB_ICONEXCLAMATION | MB_OK);
        } /* if figureDib == NULL */
        else
        {
            InvalidateRect(figureWindow, NULL, FALSE);
        } /* if figureDib == NULL */
    } /* if figurePtr != NULL */
} /* DisplayQuestion */

void GetResultsText(unsigned int *textSize, LPSTR resultsText, int detailFlag)
{
    int i;
    QuestionStruct *question;
    char tempString[512];
    int numWrongReported = 0;
    int questionGroupErrors[MAX_GROUPS];

    *textSize = 0;
    if (detailFlag)
    {
        for (i = 0; i < (numRight + numWrong); i++)
        {
            if (examQuestions[i].wrongFlag)
            {
                question = &questions[examQuestions[i].questionIndex];
                sprintf(tempString, "Question %Fs \"%Fs\" was answered incorectly.  The correct answer is \"%Fs\".\015\012\015\012",
                        question->questionNumber,
                        question->questionText,
                        question->answers[question->answerNumber]);
                *textSize += _fstrlen(tempString);
                if (resultsText != NULL)
                {
                    _fstrcat(resultsText, tempString);
                } /* if resultsText != NULL */
                numWrongReported++;
            } /* if examQuestions[i].wrongFlag */
            if (*textSize > RESULTS_TEXT_SIZE_HIWATER)
            {
                sprintf(tempString, "%d more wrong...", numWrong - numWrongReported);
                *textSize += _fstrlen(tempString);

                if (resultsText != NULL)
                {
                    _fstrcat(resultsText, tempString);
                } /* if resultsText != NULL */
                break;
            } /* if textSize > RESULTS_TEXT_SIZE_HIWATER */
        } /* for i */


    } /* if detailFlag */
    else
    {
        for (i = 0; i < MAX_GROUPS; i++)
            questionGroupErrors[i] = 0;

        for (i = 0; i < (numRight + numWrong); i++)
            if (examQuestions[i].wrongFlag)
            {
                questionGroupErrors[(questions[examQuestions[i].questionIndex].groupIndex)]++;
            }

        for (i = 0; i < numGroups; i++)
        {
            if (questionGroupErrors[i])
            {
                if (questionGroupErrors[i] == 1)
                    sprintf(tempString,
                            "Question in group %Fs \"%Fs\" answered incorrectly.\015\012\015\012",
                            questionGroups[i].groupName,
                            questionGroups[i].groupTitle);
                else
                    sprintf(tempString,
                            "%d questions in group %Fs \"%Fs\" answered incorrectly.\015\012\015\012",
                            questionGroupErrors[i],
                            questionGroups[i].groupName,
                            questionGroups[i].groupTitle);

                *textSize += _fstrlen(tempString);

                if (resultsText != NULL)
                {
                    _fstrcat(resultsText, tempString);
                } /* if resultsText != NULL */
                numWrongReported++;
            } /* if examQuestions[i].wrongFlag */
            if (*textSize > RESULTS_TEXT_SIZE_HIWATER)
            {
                sprintf(tempString, "%d more wrong...", numWrong - numWrongReported);
                *textSize += _fstrlen(tempString);

                if (resultsText != NULL)
                {
                    _fstrcat(resultsText, tempString);
                } /* if resultsText != NULL */
                break;
            } /* if textSize > RESULTS_TEXT_SIZE_HIWATER */
        } /* for i */
    } /* if detailFlag */
} /* GetResultsText() */

