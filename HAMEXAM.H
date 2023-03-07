#define APPNAME "HamExam"
#define APPNAME_RAW HamExam

#define VERSION   "2.00"
#define RVERSION   2, 0, 7, 11
#define COPYRIGHT "Copyright (C) 1994, 1995, WeGO! Software.  All Rights Reserved."

/* AboutBox Controls */
#define ABOUT_TEXT              101

/* ResultsDialog Controls */
#define SUMMARY_DETAILS_BUTTON	101
#define RESULTS_TEXT_EDIT	    102

/* PassFailDialog controls */
#define PASS_FAIL_TEXT	        101
#define RESULTS_BUTTON	        102

/* HamExam Controls */
#define QUESTION_TEXT           200
#define ANSWER_A_TEXT	        201
#define ANSWER_B_TEXT	        202
#define ANSWER_C_TEXT           203
#define ANSWER_D_TEXT           204
#define FIGURE_WINDOW           205
#define QUESTION_NUMBER_TEXT	206
#define QUESTION_OF_TEXT	    207
#define ANSWER_A_BUTTON	        208
#define ANSWER_B_BUTTON	        209
#define ANSWER_C_BUTTON	        210
#define ANSWER_D_BUTTON	        211
#define COPYRIGHT_TEXT	        212

#define WRONG_ANSWER_TEXT	100

/* Menu Tags */
#define FILE_EXIT	            101
#define EXAM_PRACTICE	        111
#define EXAM_PRACTICE_ALL	    112
#define EXAM_TEST	            113
#define QUESTIONS_NOVICE    	121
#define QUESTIONS_TECHNICIAN	122
#define QUESTIONS_GENERAL   	123
#define QUESTIONS_ADVANCED	    124
#define QUESTIONS_EXTRA	        125
#define HELP_HAMEXAM	        131
#define HELP_ABOUT	            132

/* defines for program itself */
#ifndef randomize()  /* for microsoft c */
//#define RAND_MAX 0x7FFFU
#define randomize() srand((unsigned) time(NULL))
#define random(num) (int)(((long)rand() * (num))/(RAND_MAX+1))
#endif

#define MAX_QUESTIONS           600
#define MAX_GROUPS              60

#define READ_BUFFER_SIZE        1024

#define RESULTS_TEXT_SIZE_HIWATER 50000

#define FIGURE_NAME "Figure "
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define LOAD_STATE_GROUP        1
#define LOAD_STATE_QUESTION     2
#define LOAD_STATE_ANSWER_1     3
#define LOAD_STATE_ANSWER_2     4
#define LOAD_STATE_ANSWER_3     5
#define LOAD_STATE_ANSWER_4     6

#define EXAM_MODE_IDLE          0
#define EXAM_MODE_PRACTICE      1
#define EXAM_MODE_PRACTICE_ALL  2
#define EXAM_MODE_TEST          3

/* data structures */

typedef struct {
    LPSTR groupName;            /* name of the question group */
    LPSTR groupTitle;           /* text title of group */
    int numQuestionsInGroup;    /* total number of questions in the group */
    int numQuestionsToAsk;      /* number of questions to ask from this group */
    int firstQuestionIndex;     /* index of the first questiom in this group */
} QuestionGroupStruct;

typedef struct {
    int groupIndex;             /* the index of the question group */
    LPSTR questionNumber;       /* the question number */
    LPSTR questionText;         /* the text of the question */
    LPSTR answers[4];           /* the 4 choices for answers */
    int answerNumber;           /* the index of the correct answer */
} QuestionStruct;

typedef struct {
    int questionIndex;          /* the index of this question */
    int wrongFlag;              /* true if question answered incorrectly */
} ExamQuestionsStruct;

