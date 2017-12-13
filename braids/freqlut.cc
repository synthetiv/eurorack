/*
 * Copyright 2012 Google Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Resolve frequency signal (1.0 in Q24 format = 1 octave) to phase delta.

// The LUT is just a global, and we'll need the init function to be called before
// use.

#include <stdint.h>
#include <math.h>

#include "freqlut.h"

#define LG_N_SAMPLES 10
#define N_SAMPLES (1 << LG_N_SAMPLES)
#define SAMPLE_SHIFT (24 - LG_N_SAMPLES)

#define MAX_LOGFREQ_INT 20

const int32_t lut[] = {
183251938,183376023,183500193,183624447,183748784,183873206,183997713,184122303,184246978,184371737,
184496581,184621509,184746522,184871620,184996802,185122069,185247421,185372857,185498379,185623986,
185749677,185875454,186001316,186127263,186253296,186379414,186505617,186631906,186758280,186884740,
187011285,187137916,187264633,187391436,187518324,187645298,187772359,187899505,188026738,188154056,
188281461,188408952,188536530,188664194,188791944,188919781,189047704,189175714,189303811,189431994,
189560265,189688622,189817066,189945597,190074215,190202920,190331712,190460591,190589558,190718612,
190847754,190976983,191106299,191235703,191365194,191494774,191624441,191754195,191884038,192013969,
192143987,192274094,192404288,192534571,192664942,192795402,192925949,193056586,193187310,193318123,
193449025,193580015,193711094,193842262,193973518,194104864,194236298,194367821,194499434,194631135,
194762926,194894806,195026775,195158834,195290982,195423219,195555547,195687963,195820469,195953066,
196085751,196218527,196351393,196484348,196617394,196750529,196883755,197017071,197150478,197283974,
197417561,197551239,197685007,197818865,197952815,198086855,198220985,198355207,198489519,198623922,
198758417,198893002,199027679,199162447,199297306,199432256,199567298,199702431,199837655,199972972,
200108379,200243879,200379470,200515153,200650928,200786795,200922754,201058805,201194948,201331183,
201467511,201603931,201740443,201877048,202013745,202150534,202287417,202424392,202561460,202698620,
202835874,202973220,203110659,203248192,203385817,203523536,203661348,203799254,203937252,204075345,
204213530,204351809,204490182,204628649,204767209,204905863,205044611,205183453,205322389,205461420,
205600544,205739762,205879075,206018482,206157983,206297579,206437270,206577055,206716934,206856909,
206996978,207137142,207277401,207417755,207558204,207698748,207839387,207980121,208120951,208261876,
208402897,208544013,208685224,208826531,208967934,209109433,209251027,209392718,209534504,209676386,
209818364,209960439,210102609,210244876,210387240,210529699,210672255,210814908,210957657,211100503,
211243446,211386485,211529622,211672855,211816185,211959612,212103137,212246758,212390477,212534293,
212678207,212822217,212966326,213110532,213254836,213399237,213543736,213688333,213833028,213977821,
214122712,214267701,214412788,214557974,214703258,214848640,214994120,215139699,215285377,215431153,
215577029,215723002,215869075,216015247,216161517,216307887,216454355,216600923,216747590,216894357,
217041223,217188188,217335253,217482417,217629681,217777045,217924508,218072072,218219735,218367498,
218515362,218663325,218811389,218959553,219107817,219256182,219404647,219553212,219701879,219850645,
219999513,220148481,220297551,220446721,220595992,220745365,220894838,221044413,221194089,221343866,
221493745,221643725,221793807,221943990,222094275,222244662,222395151,222545741,222696434,222847228,
222998125,223149124,223300225,223451429,223602734,223754143,223905653,224057267,224208983,224360802,
224512723,224664748,224816875,224969106,225121439,225273876,225426415,225579059,225731805,225884655,
226037608,226190666,226343826,226497090,226650459,226803931,226957507,227111186,227264970,227418858,
227572851,227726947,227881148,228035454,228189863,228344378,228498997,228653720,228808549,228963482,
229118520,229273663,229428912,229584265,229739724,229895287,230050957,230206731,230362611,230518597,
230674688,230830885,230987188,231143596,231300111,231456731,231613457,231770290,231927229,232084274,
232241425,232398683,232556047,232713518,232871096,233028780,233186571,233344468,233502473,233660585,
233818804,233977130,234135563,234294103,234452751,234611506,234770369,234929339,235088417,235247602,
235406896,235566297,235725806,235885423,236045148,236204982,236364923,236524973,236685132,236845398,
237005774,237166258,237326850,237487552,237648362,237809281,237970309,238131446,238292692,238454047,
238615512,238777085,238938769,239100561,239262464,239424476,239586597,239748829,239911170,240073621,
240236183,240398854,240561635,240724527,240887529,241050641,241213864,241377197,241540641,241704196,
241867861,242031637,242195524,242359522,242523632,242687852,242852183,243016626,243181180,243345845,
243510622,243675511,243840511,244005623,244170846,244336182,244501630,244667189,244832861,244998645,
245164541,245330549,245496670,245662903,245829249,245995708,246162279,246328963,246495760,246662670,
246829693,246996829,247164078,247331440,247498916,247666505,247834208,248002024,248169953,248337997,
248506154,248674425,248842810,249011310,249179923,249348650,249517492,249686448,249855518,250024703,
250194002,250363416,250532945,250702589,250872347,251042220,251212209,251382312,251552531,251722865,
251893314,252063879,252234559,252405355,252576266,252747293,252918436,253089695,253261070,253432561,
253604168,253775891,253947730,254119686,254291758,254463947,254636253,254808675,254981213,255153869,
255326642,255499531,255672538,255845661,256018902,256192261,256365736,256539329,256713040,256886868,
257060814,257234878,257409060,257583359,257757777,257932312,258106966,258281738,258456629,258631638,
258806765,258982011,259157376,259332859,259508462,259684183,259860023,260035982,260212060,260388258,
260564575,260741011,260917567,261094242,261271037,261447952,261624986,261802141,261979415,262156809,
262334324,262511958,262689713,262867588,263045584,263223700,263401937,263580295,263758773,263937372,
264116092,264294933,264473896,264652979,264832184,265011510,265190957,265370526,265550216,265730029,
265909963,266090018,266270196,266450496,266630918,266811462,266992128,267172916,267353827,267534861,
267716017,267897296,268078697,268260222,268441869,268623639,268805532,268987549,269169688,269351952,
269534338,269716848,269899482,270082239,270265120,270448125,270631253,270814506,270997883,271181384,
271365009,271548759,271732633,271916631,272100754,272285002,272469375,272653872,272838494,273023242,
273208114,273393112,273578234,273763483,273948856,274134355,274319980,274505731,274691607,274877609,
275063737,275249991,275436371,275622878,275809511,275996270,276183155,276370167,276557306,276744572,
276931964,277119483,277307129,277494902,277682802,277870830,278058985,278247267,278435677,278624214,
278812879,279001672,279190593,279379641,279568818,279758122,279947555,280137116,280326806,280516624,
280706570,280896645,281086849,281277181,281467643,281658233,281848952,282039801,282230779,282421886,
282613123,282804489,282995984,283187609,283379364,283571249,283763264,283955409,284147684,284340089,
284532624,284725290,284918086,285111013,285304071,285497259,285690578,285884027,286077608,286271320,
286465163,286659138,286853243,287047481,287241849,287436349,287630981,287825745,288020641,288215668,
288410828,288606120,288801544,288997100,289192789,289388611,289584564,289780651,289976870,290173223,
290369708,290566326,290763078,290959962,291156980,291354131,291551416,291748835,291946387,292144073,
292341892,292539846,292737934,292936155,293134512,293333002,293531627,293730386,293929280,294128308,
294327472,294526770,294726203,294925771,295125474,295325313,295525287,295725396,295925641,296126021,
296326537,296527189,296727977,296928900,297129960,297331156,297532488,297733956,297935561,298137302,
298339180,298541195,298743346,298945634,299148060,299350622,299553322,299756158,299959132,300162244,
300365493,300568880,300772404,300976066,301179866,301383805,301587881,301792095,301996448,302200939,
302405568,302610337,302815243,303020289,303225473,303430796,303636259,303841860,304047601,304253481,
304459500,304665659,304871957,305078396,305284974,305491691,305698549,305905547,306112685,306319963,
306527382,306734941,306942641,307150481,307358462,307566584,307774847,307983251,308191795,308400482,
308609309,308818278,309027388,309236640,309446034,309655569,309865246,310075066,310285027,310495131,
310705376,310915764,311126295,311336968,311547784,311758743,311969844,312181088,312392476,312604006,
312815680,313027497,313239458,313451562,313663809,313876201,314088736,314301415,314514238,314727206,
314940317,315153573,315366973,315580518,315794207,316008041,316222020,316436143,316650412,316864825,
317079384,317294089,317508938,317723933,317939074,318154360,318369792,318585370,318801094,319016964,
319232980,319449143,319665452,319881907,320098509,320315257,320532153,320749195,320966384,321183720,
321401203,321618834,321836612,322054537,322272610,322490831,322709200,322927716,323146380,323365192,
323584153,323803262,324022519,324241924,324461479,324681181,324901033,325121033,325341183,325561481,
325781929,326002526,326223272,326444168,326665213,326886408,327107753,327329248,327550893,327772687,
327994632,328216728,328438973,328661369,328883916,329106613,329329462,329552461,329775611,329998912,
330222364,330445968,330669723,330893630,331117688,331341898,331566260,331790773,332015439,332240257,
332465227,332690350,332915624,333141052,333366632,333592365,333818250,334044289,334270480,334496825,
334723323,334949975,335176780,335403738,335630850,335858116,336085536,336313110,336540838,336768720,
336996756,337224947,337453293,337681793,337910447,338139257,338368221,338597341,338826615,339056045,
339285631,339515371,339745268,339975320,340205527,340435891,340666410,340897086,341127918,341358906,
341590051,341821352,342052810,342284424,342516195,342748124,342980209,343212451,343444851,343677408,
343910123,344142995,344376024,344609212,344842558,345076061,345309723,345543542,345777521,346011657,
346245952,346480406,346715019,346949790,347184720,347419810,347655058,347890466,348126034,348361760,
348597647,348833693,349069899,349306265,349542791,349779477,350016323,350253330,350490498,350727825,
350965314,351202963,351440774,351678745,351916877,352155171,352393626,352632243,352871021,353109961,
353349062,353588326,353827751,354067339,354307089,354547001,354787076,355027313,355267713,355508275,
355749001,355989890,356230941,356472156,356713534,356955076,357196781,357438650,357680683,357922879,
358165240,358407765,358650454,358893307,359136325,359379507,359622854,359866365,360110042,360353883,
360597890,360842062,361086399,361330902,361575570,361820404,362065404,362310569,362555901,362801399,
363047063,363292893,363538890,363785053,364031383,364277880,364524544,364771374,365018372,365265537,
365512870,365760370,366008038,366255873,366503876};
void Freqlut::init(double sample_rate) {
}

// Note: if logfreq is more than 20.0, the results will be inaccurate. However,
// that will be many times the Nyquist rate.
int32_t Freqlut::lookup(int32_t logfreq) {
  int ix = (logfreq & 0xffffff) >> SAMPLE_SHIFT;

  int32_t y0 = lut[ix];
  int32_t y1 = lut[ix + 1];
  int lowbits = logfreq & ((1 << SAMPLE_SHIFT) - 1);
  int32_t y = y0 + ((((int64_t)(y1 - y0) * (int64_t)lowbits)) >> SAMPLE_SHIFT);
  int hibits = logfreq >> 24;
  return y >> (MAX_LOGFREQ_INT - hibits);
}
