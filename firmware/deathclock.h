/***************************************************************************
 MonoChron Death Clock firmware July 28, 2010
 (c) 2010 Damien Good

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#define DC_mode_normal 0
#define DC_mode_pessimistic 1
#define DC_mode_optimistic 2
#define DC_mode_sadistic 3

#define DC_gender_male 0
#define DC_gender_female 1

#define DC_non_smoker 0
#define DC_smoker 1

#define BMI_Imperial 0
#define BMI_Metric 1
#define BMI_Direct 2

uint32_t date_diff ( uint8_t month1, uint8_t day1, uint8_t year1, uint8_t month2, uint8_t day2, uint8_t year2 );
uint8_t BodyMassIndex ( uint8_t unit, uint16_t height, uint16_t weight );
uint32_t ETD ( uint8_t DOB_month, 
               uint8_t DOB_day, 
               uint8_t DOB_year, 
               uint8_t month, 
               uint8_t day, 
               uint8_t year, 
               uint8_t Gender, 
               uint8_t Mode, 
               uint8_t BMI, 
               uint8_t Smoker, 
               uint8_t hour,
               uint8_t min,
               uint8_t sec);