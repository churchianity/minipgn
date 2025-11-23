
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // memset

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <profileapi.h>
#endif

// if you need thread-safety because you are intending to parse PGN concurrently/in 
// multiple threads at once, you can define this:
//#define MPGN_DO_NOT_USE_GLOBAL_ERROR_BUFFER

#define MINIPGN_IMPLEMENTATION
#include "mpgn.h"


// movetext is multi-line, contains "3...", contains a mid-movetext comment, etc.
static const char *test1 =
"[Event \"F/S Return Match\"]\n"
"[Site \"Belgrade, Serbia JUG\"]\n"
"[Date \"1992.11.04\"]\n"
"[Round \"29\"]\n"
"[White \"Fischer, Robert J.\"]\n"
"[Black \"Spassky, Boris V.\"]\n"
"[Result \"1/2-1/2\"]\n"
"\n"
"1.e4 e5 2.Nf3 Nc6 3.Bb5 {This opening is called the Ruy Lopez.} 3...a6\n"
"4.Ba4 Nf6 5.O-O Be7 6.Re1 b5 7.Bb3 d6 8.c3 O-O 9.h3 Nb8 10.d4 Nbd7\n"
"11.c4 c6 12.cxb5 axb5 13.Nc3 Bb7 14.Bg5 b4 15.Nb1 h6 16.Bh4 c5 17.dxe5\n"
"Nxe4 18.Bxe7 Qxe7 19.exd6 Qf6 20.Nbd2 Nxd6 21.Nc4 Nxc4 22.Bxc4 Nb6\n"
"23.Ne5 Rae8 24.Bxf7+ Rxf7 25.Nxf7 Rxe1+ 26.Qxe1 Kxf7 27.Qe3 Qg5 28.Qxg5\n"
"hxg5 29.b3 Ke6 30.a3 Kd6 31.axb4 cxb4 32.Ra5 Nd5 33.f3 Bc8 34.Kf2 Bf5\n"
"35.Ra7 g6 36.Ra6+ Kc5 37.Ke1 Nf4 38.g3 Nxh3 39.Kd2 Kb5 40.Rd6 Kc5 41.Ra6\n"
"Nf2 42.g4 Bd3 43.Re6 1/2-1/2\n";

// white space between moves, queen promotion, long final move 'Qexg6#'
static const char *test2 =
"[Event \"Rated Blitz game\"]\n"
"[Site \"https://lichess.org/z2ncoii6\"]\n"
"[White \"Abd0\"]\n"
"[Black \"Killi\"]\n"
"[Result \"1-0\"]\n"
"[UTCDate \"2012.12.31\"]\n"
"[UTCTime \"23:08:21\"]\n"
"[WhiteElo \"1436\"]\n"
"[BlackElo \"1506\"]\n"
"[WhiteRatingDiff \"+15\"]\n"
"[BlackRatingDiff \"-14\"]\n"
"[ECO \"C60\"]\n"
"[Opening \"Ruy Lopez: Cozio Defense\"]\n"
"[TimeControl \"420+0\"]\n"
"[Termination \"Normal\"]\n"
"\n"
"1. e4 e5 2. Nf3 Nc6 3. Bb5 Nge7 4. Nc3 h6 5. Nd5 a6 6. Ba4 b5 7. Bb3 Ng6 8. c4 b4 9. Ba4 Nd4 10. O-O c6 11. Ne3 a5 12. Nxd4 exd4 13. Nf5 Ba6 14. Nxd4 Bxc4 15. Nxc6 Bxf1 16. Nxd8 Bxg2 17. Nxf7 Bxe4 18. Qe2 Kxf7 19. Qxe4 Kg8 20. Qxa8 Kh7 21. Qe4 d5 22. Qxd5 Be7 23. d3 Rf8 24. Bb3 Nf4 25. Qe4+ Ng6 26. Be3 Bh4 27. Rf1 Bf6 28. Bd4 Bh4 29. f4 Bd8 30. f5 Ne7 31. f6+ Ng6 32. fxg7 Rxf1+ 33. Kxf1 h5 34. g8=Q+ Kh6 35. Qexg6# 1-0\n";

// long move/disambiguation, 'Nb5xd5'
static const char *test3 =
"[Event \"Rated Blitz game\"]\n"
"[Site \"https://lichess.org/4sv62xky\"]\n"
"[White \"tiggran\"]\n"
"[Black \"rasmussenesq\"]\n"
"[Result \"1-0\"]\n"
"[UTCDate \"2013.01.01\"]\n"
"[UTCTime \"00:03:20\"]\n"
"[WhiteElo \"1540\"]\n"
"[BlackElo \"1653\"]\n"
"[WhiteRatingDiff \"+15\"]\n"
"[BlackRatingDiff \"-15\"]\n"
"[ECO \"B44\"]\n"
"[Opening \"Sicilian Defense: Paulsen Variation\"]\n"
"[TimeControl \"300+0\"]\n"
"[Termination \"Normal\"]\n"
"\n"
"1. e4 c5 2. Nf3 e6 3. d4 cxd4 4. Nxd4 Nc6 5. Nb3 d5 6. exd5 Qxd5 7. Qxd5 exd5 8. Nc3 Nf6 9. Be2 Be6 10. O-O d4 11. Nb5 Rc8 12. Nb5xd4 Nxd4 13. Nxd4 Bc5 14. c3 O-O 15. Be3 Bb6 16. Bg5 Nd5 17. Nxe6 fxe6 18. Be3 Nxe3 19. fxe3 Bxe3+ 20. Kh1 Rf2 21. Rxf2 Bxf2 22. Bf3 Rc7 23. Rf1 Bc5 24. Bg4 e5 25. Rf5 Re7 26. Rf1 b6 27. Bd1 e4 28. Bb3+ Kh8 29. Rf8# 1-0\n";

// each move has a comment after with the engine evaluation noted, liberal usage of '...', CSMs in use, like '?!'.
static const char *test4 =
"[Event \"Rated Bullet game\"]\n"
"[Site \"https://lichess.org/hca0mb9v\"]\n"
"[White \"LEGENDARY_ERFAN\"]\n"
"[Black \"Mariss\"]\n"
"[Result \"0-1\"]\n"
"[UTCDate \"2013.01.01\"]\n"
"[UTCTime \"00:15:38\"]\n"
"[WhiteElo \"1182\"]\n"
"[BlackElo \"1457\"]\n"
"[WhiteRatingDiff \"-30\"]\n"
"[BlackRatingDiff \"+5\"]\n"
"[ECO \"C00\"]\n"
"[Opening \"French Defense #2\"]\n"
"[TimeControl \"60+0\"]\n"
"[Termination \"Normal\"]\n"
"\n"
"1. e4 { [%eval 0.2] } 1... e6 { [%eval 0.13] } 2. Bc4 { [%eval -0.31] } 2... d5 { [%eval -0.28] } 3. exd5 { [%eval -0.37] } 3... exd5 { [%eval -0.31] } 4. Bb3 { [%eval -0.33] } 4... Nf6 { [%eval -0.35] } 5. d4 { [%eval -0.34] } 5... Be7 { [%eval 0.0] } 6. Nf3 { [%eval 0.0] } 6... O-O { [%eval -0.08] } 7. Bg5 { [%eval -0.19] } 7... h6 { [%eval -0.29] } 8. Bxf6 { [%eval -0.36] } 8... Bxf6 { [%eval -0.37] } 9. O-O { [%eval -0.36] } 9... c6 { [%eval -0.12] } 10. Re1 { [%eval -0.17] } 10... Bf5 { [%eval -0.04] } 11. c4?! { [%eval -0.67] } 11... dxc4 { [%eval -0.5] } 12. Bxc4 { [%eval -0.77] } 12... Nd7?! { [%eval -0.1] } 13. Nc3 { [%eval 0.0] } 13... Nb6 { [%eval 0.0] } 14. b3?! { [%eval -0.76] } 14... Nxc4 { [%eval -0.49] } 15. bxc4 { [%eval -0.65] } 15... Qa5 { [%eval -0.55] } 16. Rc1 { [%eval -0.79] } 16... Rad8 { [%eval -0.78] } 17. d5?? { [%eval -5.41] } 17... Bxc3 { [%eval -5.42] } 18. Re5? { [%eval -7.61] } 18... Bxe5 { [%eval -7.78] } 19. Nxe5 { [%eval -7.72] } 19... cxd5 { [%eval -7.81] } 20. Qe1? { [%eval -9.29] } 20... Be6?? { [%eval 3.71] } 21. Rd1?? { [%eval -12.34] } 21... dxc4 { [%eval -12.71] } 22. Rxd8?! { [%eval #-1] } 22... Rxd8?! { [%eval -13.06] } 23. Qc3?! { [%eval #-2] } 23... Qxc3?! { [%eval #-4] } 24. g3 { [%eval #-3] } 24... Rd1+?! { [%eval #-4] } 25. Kg2 { [%eval #-4] } 25... Qe1?! { [%eval #-4] } 26. Kf3 { [%eval #-3] } 26... Qxe5 { [%eval #-2] } 27. Kg2 { [%eval #-2] } 27... Bd5+?! { [%eval #-2] } 28. Kh3 { [%eval #-1] } 28... Qh5# 0-1\n";

// CSM on the end of a castle 'O-O?!', eval comments, etc.
static const char *test5 =
"[Event \"Rated Blitz game\"]\n"
"[Site \"https://lichess.org/z6414m4b\"]\n"
"[White \"psonio\"]\n"
"[Black \"Elquesoy\"]\n"
"[Result \"1-0\"]\n"
"[UTCDate \"2013.01.01\"]\n"
"[UTCTime \"06:43:28\"]\n"
"[WhiteElo \"1847\"]\n"
"[BlackElo \"1424\"]\n"
"[WhiteRatingDiff \"+6\"]\n"
"[BlackRatingDiff \"-4\"]\n"
"[ECO \"C65\"]\n"
"[Opening \"Ruy Lopez: Berlin Defense, Beverwijk Variation\"]\n"
"[TimeControl \"240+0\"]\n"
"[Termination \"Normal\"]\n"
"\n"
"1. e4 { [%eval 0.21] } 1... e5 { [%eval 0.25] } 2. Nf3 { [%eval 0.3] } 2... Nc6 { [%eval 0.31] } 3. Bb5 { [%eval 0.15] } 3... Bc5 { [%eval 0.44] } 4. O-O { [%eval 0.46] } 4... Nf6 { [%eval 0.39] } 5. d3 { [%eval -0.03] } 5... O-O?! { [%eval 0.68] } 6. Bxc6 { [%eval 0.7] } 6... dxc6 { [%eval 0.91] } 7. Nxe5 { [%eval 0.88] } 7... Qd4 { [%eval 1.13] } 8. Nc4?! { [%eval 0.31] } 8... Ng4?! { [%eval 1.22] } 9. Ne3?! { [%eval 0.68] } 9... Nxe3 { [%eval 0.66] } 10. fxe3 { [%eval 0.67] } 10... Qe5? { [%eval 2.22] } 11. d4 { [%eval 2.3] } 11... Bxd4 { [%eval 2.78] } 12. exd4 { [%eval 2.74] } 12... Qxe4 { [%eval 2.63] } 13. Nc3 { [%eval 2.61] } 13... Qg6 { [%eval 2.63] } 14. Qd3?! { [%eval 1.97] } 14... Bf5?? { [%eval 6.61] } 15. Qxf5 { [%eval 6.34] } 1-0\n";

// complex move disambiguation that uses rank rather than file: 'N2xf3'
static const char *test6 = 
"[Event \"Rated Blitz tournament https://lichess.org/tournament/qg3Tx2iL\"]\n"
"[Site \"https://lichess.org/I40vIrwH\"]\n"
"[White \"hachazo_negro\"]\n"
"[Black \"rjzi\"]\n"
"[Result \"0-1\"]\n"
"[UTCDate \"2015.07.31\"]\n"
"[UTCTime \"22:01:21\"]\n"
"[WhiteElo \"1645\"]\n"
"[BlackElo \"1690\"]\n"
"[WhiteRatingDiff \"-10\"]\n"
"[BlackRatingDiff \"+10\"]\n"
"[ECO \"B41\"]\n"
"[Opening \"Sicilian Defense: Kan Variation\"]\n"
"[TimeControl \"300+0\"]\n"
"[Termination \"Normal\"]\n"
"\n"
"1. e4 { [%eval 0.12] } 1... c5 { [%eval 0.25] } 2. Nf3 { [%eval 0.25] } 2... e6 { [%eval 0.2] } 3. d4 { [%eval 0.23] } 3... cxd4 { [%eval 0.19] } 4. Nxd4 { [%eval 0.25] } 4... a6 { [%eval 0.38] } 5. c3 { [%eval 0.37] } 5... b5 { [%eval 0.7] } 6. Be2 { [%eval 0.4] } 6... Bb7 { [%eval 0.29] } 7. Bf3 { [%eval 0.17] } 7... Qc7 { [%eval 0.22] } 8. Be3 { [%eval -0.02] } 8... Nf6 { [%eval 0.01] } 9. Nd2 { [%eval 0.0] } 9... Be7 { [%eval 0.08] } 10. O-O { [%eval 0.13] } 10... O-O { [%eval 0.13] } 11. e5?! { [%eval -0.4] } 11... Bxf3 { [%eval 0.0] } 12. N2xf3 { [%eval 0.0] } 12... Nd5 { [%eval 0.0] } 13. Qd2 { [%eval -0.31] } 13... Nxe3 { [%eval -0.05] } 14. fxe3 { [%eval -0.41] } 14... d6 { [%eval 0.07] } 15. exd6 { [%eval 0.06] } 15... Bxd6 { [%eval 0.09] } 16. g3 { [%eval -0.27] } 16... Nd7 { [%eval -0.22] } 17. Qd3 { [%eval -0.46] } 17... Nc5 { [%eval -0.38] } 18. Qc2 { [%eval -0.51] } 18... Rad8?! { [%eval 0.0] } 19. b4 { [%eval 0.1] } 19... Na4 { [%eval 0.03] } 20. Ng5 { [%eval 0.01] } 20... g6 { [%eval 0.03] } 21. Rac1? { [%eval -1.13] } 21... Qc4 { [%eval -0.74] } 22. Ne4 { [%eval -0.93] } 22... Be5 { [%eval -0.86] } 23. Ne2?! { [%eval -1.56] } 23... Kg7?! { [%eval -0.84] } 24. a3?! { [%eval -1.44] } 24... Rd3?! { [%eval -0.65] } 25. Rcd1?? { [%eval -5.86] } 25... Rxe3 { [%eval -5.87] } 26. Nd2?! { [%eval -6.74] } 26... Qg4?! { [%eval -5.89] } 27. Rf2? { [%eval -8.67] } 27... Nxc3?? { [%eval -5.65] } 28. Nxc3 { [%eval -5.6] } 28... Rxc3 { [%eval -5.5] } 29. Qb1 { [%eval -5.6] } 29... Rxa3 { [%eval -5.71] } 30. Re1?! { [%eval -6.56] } 30... Bf6?? { [%eval -1.05] } 31. Qc1?? { [%eval -5.73] } 31... Ra1 { [%eval -5.61] } 32. Nb1? { [%eval -8.03] } 32... Qxb4? { [%eval -6.22] } 33. Ref1? { [%eval -8.97] } 33... Bd4 { [%eval -9.13] } 34. Qf4?! { [%eval -11.16] } 34... Bxf2+ { [%eval -11.18] } 35. Qxf2 { [%eval -16.08] } 35... Rxb1 { [%eval -12.43] } 36. Qf6+ { [%eval -48.03] } 36... Kg8 { [%eval -19.21] } 37. Rxb1 { [%eval -48.03] } 37... Qxb1+ { [%eval -24.07] } 38. Kg2 { [%eval -19.68] } 38... Qe4+ { [%eval -57.84] } 39. Kh3 { [%eval -23.03] } 39... Qf5+ { [%eval #-12] } 0-1\n";

// large movetext section, usage of "1. ..." instead of "1...", clk comments
static const char *test7 =
"[Event \"Roquetas Chess Festival 2025 - XXXVI International Open\"]\n"
"[Site \"?\"]\n"
"[Date \"2025.01.04\"]\n"
"[Round \"5\"]\n"
"[White \"Domingo Nunez, Alejandro\"]\n"
"[Black \"Novais, Carlos Joao Fernandes\"]\n"
"[Result \"1-0\"]\n"
"[WhiteElo \"2316\"]\n"
"[BlackElo \"2125\"]\n"
"[ECO \"A49\"]\n"
"[WhiteTitle \"FM\"]\n"
"[Source \"LichessBroadcast\"]\n"
"[ImportDate \"2025-01-07\"]\n"
"[WhiteFideId \"22245561\"]\n"
"[BlackFideId \"1904450\"]\n"
"\n"
"1. Nf3 {[%clk 1:28:30]} 1. ... Nf6 {[%clk 1:30:46]} 2. g3 {[%clk\n"
"1:28:57]} 2. ... g6 {[%clk 1:31:06]} 3. b3 {[%clk 1:29:25]} 3. ... Bg7 {\n"
"[%clk 1:31:21]} 4. Bb2 {[%clk 1:29:54]} 4. ... O-O {[%clk 1:31:33]} 5. Bg2\n"
"{[%clk 1:30:22]} 5. ... d6 {[%clk 1:31:51]} 6. d4 {[%clk 1:30:49]} 6. ... \n"
"e5 {[%clk 1:31:55]} 7. dxe5 {[%clk 1:31:16]} 7. ... Nfd7 {[%clk\n"
"1:32:18]} 8. c4 {[%clk 1:28:58]} 8. ... dxe5 {[%clk 1:32:22]} 9. Qc1 {\n"
"[%clk 1:23:25]} 9. ... e4 {[%clk 1:26:00]} 10. Nd4 {[%clk 1:23:54]} \n"
"10. ... Re8 {[%clk 1:25:26]} 11. O-O {[%clk 1:24:06]} 11. ... Na6 {[%clk\n"
"1:22:56]} 12. Nc2 {[%clk 1:23:26]} 12. ... Nac5 {[%clk 1:17:37]} 13. Bxg7 \n"
"{[%clk 1:21:58]} 13. ... Kxg7 {[%clk 1:17:59]} 14. Nc3 {[%clk\n"
"1:22:13]} 14. ... a5 {[%clk 1:18:18]} 15. Qe3 {[%clk 1:15:27]} 15. ... c6 \n"
"{[%clk 1:09:43]} 16. Rad1 {[%clk 1:14:41]} 16. ... Qe7 {[%clk 1:10:03]} \n"
"17. Rd4 {[%clk 1:11:08]} 17. ... Nf6 {[%clk 1:10:18]} 18. h3 {[%clk\n"
"1:10:52]} 18. ... h5 {[%clk 1:07:54]} 19. Rfd1 {[%clk 1:09:37]} 19. ... \n"
"Bf5 {[%clk 1:07:59]} 20. R4d2 {[%clk 1:06:17]} 20. ... Qe5 {[%clk\n"
"0:59:45]} 21. Qd4 {[%clk 0:48:09]} 21. ... Qxd4 {[%clk 0:54:04]} 22. Rxd4 \n"
"{[%clk 0:48:32]} 22. ... Ne6 {[%clk 0:53:42]} 23. Rd6 {[%clk\n"
"0:47:37]} 23. ... g5 {[%clk 0:50:14]} 24. Ne3 {[%clk 0:46:41]} 24. ... Bg6\n"
"{[%clk 0:50:35]} 25. Na4 {[%clk 0:42:17]} 25. ... Red8 {[%clk 0:44:59]} \n"
"26. c5 {[%clk 0:39:57]} 26. ... Rxd6 {[%clk 0:45:13]} 27. Rxd6 {[%clk\n"
"0:39:59]} 27. ... Kf8 {[%clk 0:45:27]} 28. Nc4 {[%clk 0:40:14]} 28. ... \n"
"Ke7 {[%clk 0:45:35]} 29. Ncb6 {[%clk 0:24:56]} 29. ... Rd8 {[%clk\n"
"0:41:24]} 30. Rxd8 {[%clk 0:25:03]} 30. ... Kxd8 {[%clk 0:41:44]} 31. Nc4 \n"
"{[%clk 0:25:24]} 31. ... Nd7 {[%clk 0:42:04]} 32. Nxa5 {[%clk\n"
"0:25:47]} 32. ... Kc7 {[%clk 0:42:25]} 33. b4 {[%clk 0:26:12]} 33. ... Nd4\n"
"{[%clk 0:42:47]} 34. Nc3 {[%clk 0:20:34]} 34. ... Nc2 {[%clk 0:37:42]} 35.\n"
" Bxe4 {[%clk 0:13:26]} 35. ... Nxb4 {[%clk 0:37:35]} 36. Bxg6 {[%clk\n"
"0:13:44]} 36. ... fxg6 {[%clk 0:37:59]} 37. Nb3 {[%clk 0:14:11]} 37. ... \n"
"Ne5 {[%clk 0:36:17]} 38. f3 {[%clk 0:10:02]} 38. ... b6 {[%clk 0:32:58]} \n"
"39. cxb6+ {[%clk 0:07:59]} 39. ... Kxb6 {[%clk 0:33:18]} 40. Kf2 {[%clk \n"
"0:08:11]} 40. ... Nc2 {[%clk 0:29:45]} 41. f4 {[%clk 0:03:16]} 41. ... \n"
"gxf4 {[%clk 0:29:10]} 42. gxf4 {[%clk 0:03:45]} 42. ... Nd7 {[%clk\n"
"0:28:54]} 43. e4 {[%clk 0:01:59]} 43. ... c5 {[%clk 0:29:08]} 44. e5 {\n"
"[%clk 0:01:23]} 44. ... c4 {[%clk 0:29:15]} 45. Nd2 {[%clk 0:01:48]} \n"
"45. ... Kc5 {[%clk 0:28:36]} 46. Nde4+ {[%clk 0:00:57]} 46. ... Kc6 {[%clk\n"
"0:28:34]} 47. a4 {[%clk 0:00:33]} 47. ... Nd4 {[%clk 0:27:13]} 48. Nd6 {\n"
"[%clk 0:00:33]} 48. ... Kc5 {[%clk 0:25:54]} 49. Ke3 {[%clk 0:00:42]} \n"
"49. ... Nb3 {[%clk 0:24:26]} 50. Nde4+ {[%clk 0:00:37]} 50. ... Kc6 {[%clk\n"
"0:24:40]} 51. Ne2 {[%clk 0:00:34]} 51. ... Nf8 {[%clk 0:22:12]} 52. Ng5 {\n"
"[%clk 0:00:33]} 52. ... Nc5 {[%clk 0:20:24]} 53. Nc3 {[%clk 0:00:37]} \n"
"53. ... Nce6 {[%clk 0:20:02]} 54. Nxe6 {[%clk 0:00:44]} 54. ... Nxe6 {\n"
"[%clk 0:20:27]} 55. Ne4 {[%clk 0:00:59]} 55. ... Kd5 {[%clk 0:20:08]} 56. \n"
"a5 {[%clk 0:00:34]} 56. ... Ng7 {[%clk 0:19:27]} 57. Nd6 {[%clk\n"
"0:00:50]} 57. ... Kc5 {[%clk 0:17:40]} 58. a6 {[%clk 0:00:55]} 58. ... Kb6\n"
"{[%clk 0:17:49]} 59. Nxc4+ {[%clk 0:00:51]} 59. ... Kxa6 {[%clk 0:18:14]} \n"
"60. Ke4 {[%clk 0:00:52]} 60. ... Kb7 {[%clk 0:18:38]} 61. Kd5 {[%clk\n"
"0:00:38]} 61. ... Kc7 {[%clk 0:18:30]} 62. e6 {[%clk 0:00:39]} 62. ... Kd8\n"
"{[%clk 0:18:36]} 63. Kd6 {[%clk 0:00:36]} 63. ... Ke8 {[%clk 0:17:51]} 64.\n"
" Ne3 {[%clk 0:00:55]} 64. ... Kd8 {[%clk 0:15:51]} 65. Nd5 {[%clk\n"
"0:01:08]} 65. ... Ne8+ {[%clk 0:15:33]} 66. Ke5 {[%clk 0:01:16]} 66. ... \n"
"Kc8 {[%clk 0:15:58]} 67. Ne7+ {[%clk 0:01:37]} 1-0\n";

// contains '(', some CSMs '!' etc.
const char *test8 =
"[Event \"Prague Open B\"]\n"
"[Site \"?\"]\n"
"[Date \"2025.01.10\"]\n"
"[Round \"1.1\"]\n"
"[White \"Illandzis, Spyridon\"]\n"
"[Black \"Kotvalt, Antonin\"]\n"
"[Result \"1-0\"]\n"
"[WhiteElo \"1950\"]\n"
"[BlackElo \"1618\"]\n"
"[ECO \"A01\"]\n"
"[EventDate \"2025.01.10\"]\n"
"[PlyCount \"29\"]\n"
"[Source \"GreekBase\"]\n"
"[ImportDate \"2025-06-03\"]\n"
"[BlackFideId \"23753722\"]\n"
"\n"
"1. b3 d5 2. Bb2 c5 3. e3 e6 4. f4 Nf6 5. Nf3 Be7 6. Bb5+ Nc6 ( 6. ... Bd7 \n"
") ( 6. ... Nbd7 ) 7. O-O O-O 8. Bxc6 bxc6 9. Ne5 Qb6 10. Rf3 Rd8?! {(6' \n"
" 18')} 11. Rg3 Bf8? {[#]} ( 11. ... c4!? ) 12. Nc4! dxc4 13. Bxf6 Rd7 {\n"
"[#]} 14. Rxg7+! ( 14. Bxg7? {he was hoping for} 14. ... Bxg7 15. Qg4 f5 ) \n"
"14. ... Kh8 ( 14. ... Bxg7 15. Qg4 Kf8 16. Qxg7+ Ke8 17. Qg8# ) 15. Rg8+! \n"
"1-0\n";

// game with an unknown result. '*'
const char *test9 =
"[Event \"29th European Blitz\"]\n"
"[Site \"Warsaw POL\"]\n"
"[Date \"2010.12.17\"]\n"
"[Round \"10\"]\n"
"[White \"Vachier Lagrave,M\"]\n"
"[Black \"Ivanchuk,V\"]\n"
"[Result \"*\"]\n"
"[WhiteElo \"2703\"]\n"
"[BlackElo \"2764\"]\n"
"[ECO \"C80\"]\n"
"\n"
"1.e4 e5 2.Nf3 Nc6 3.Bb5 Nf6 4.O-O Nxe4 5.d4 a6 6.Ba4 b5 7.Bb3 d5 8.dxe5 Be6\n"
"9.Nbd2 Nc5 10.c3 d4 11.Bxe6 Nxe6 12.cxd4 Ncxd4 13.a4 Bc5 14.Ne4 Bb6 15.axb5 axb5\n"
"16.Rxa8 Qxa8 17.Nxd4 Qxe4 18.Nxe6 fxe6 19.Be3 Bxe3 20.fxe3 Qxe5 21.Qf3 Ke7\n"
"22.Kh1 Qf5 23.Qe2 Qd5 24.e4 Qe5 25.Qf2 Ra8 26.h3 h6 27.Qf7+ Kd6 28.Rc1 Rc8\n"
"29.b4 g5 30.Qh7 Qf4 31.Rd1+ Kc6 32.Qd7+  *\n";

// null move. A 'null' move indicates a 'passed' or 'skipped' move. It's not supported by PGN, but nontheless some PGN data contains them.
const char *test10 = 
"[Date \"2016.11.25\"]\n"
"[Round \"2.33\"]\n"
"[White \"Mihajlovic,Mio\"]\n"
"[Black \"Smolovic,M\"]\n"
"[Result \"0-1\"]\n"
"[WhiteElo \"2098\"]\n"
"[BlackElo \"2298\"]\n"
"[ECO \"A00\"]\n"
"[EventDate \"2016.11.24\"]\n"
"[WhiteTitle \"FM\"]\n"
"[BlackTitle \"IM\"]\n"
"[WhiteFideId \"906301\"]\n"
"[BlackFideId \"915165\"]\n"
"\n"
"1.--\n"
"0-1\n";

// tag "Variation" contains brackets
const char *test11 =
"[Event \"47th Ilmar Raud Mem\"]\n"
"[Site \"Viljandi EST\"]\n"
"[Date \"2013.07.02\"]\n"
"[Round \"3.20\"]\n"
"[White \"Soot,M\"]\n"
"[Black \"Tominga,Ago\"]\n"
"[Result \"1-0\"]\n"
"[WhiteElo \"2116\"]\n"
"[ECO \"C36\"]\n"
"[EventDate \"2013.07.01\"]\n"
"[Opening \"KGA\"]\n"
"[Variation \"Abbazia defence (classical defence, modern defence[!])\"]\n"
"[WhiteFideId \"4501160\"]\n"
"[BlackFideId \"4504127\"]\n"
"\n"
"1.e4 e5 2.f4 d5 3.Nf3 exf4 4.exd5 Qxd5 5.Nc3 Qa5 6.d4 Bd6 7.Bc4 Ne7 8.Bd2 c5\n"
"9.Ng5 Be6 10.Nxe6 fxe6 11.Bxe6 cxd4 12.Nd5 Qa6 13.Qh5+ g6 14.Nf6+ Kd8 15.Qh3\n"
"Nbc6 16.Bc4\n"
"1-0\n";

// Event and Site contains escaped quotes.
const char *test12 =
"[Event \"Tubingen 2\\\"\"]\n"
"[Site \"Tubingen 2\\\"\"]\n"
"[Date \"1981.??.??\"]\n"
"[Round \"1\"]\n"
"[White \"Schlenker, R. (wh)\"]\n"
"[Black \"Frick, C. (bl)\"]\n"
"[Result \"1-0\"]\n"
"[ECO \"C34\"]\n"
"\n"
"1.e4 e5 2.f4 exf4 3.Nf3 Nf6 4.e5 Nh5 5.d4 g5 6.Bc4 g4 7.O-O d5 8.exd6 Bxd6\n"
"9.Re1+ Kf8 10.g3 fxg3 11.Bh6+ Kg8 12.Qd2 gxf3 13.Re8+ Bf8 14.Qg5+ Ng7 15.Qxg7#\n"
"1-0\n";

// 'Event' tag contains an escaped backslash but not an escaped ending quote
const char *test13 =
"[Event \"It \\\\\"]\n"
"[Site \"Doha (Qatar)\"]\n"
"[Date \"2002.??.??\"]\n"
"[Round \"8\"]\n"
"[White \"Belkhodja, Slim\"]\n"
"[Black \"El Taher, Fouad\"]\n"
"[Result \"1/2-1/2\"]\n"
"[WhiteElo \"2506\"]\n"
"[BlackElo \"2502\"]\n"
"[ECO \"A05\"]\n"
"\n"
"1.d3 Nf6 2.Nf3 d6 3.g3 g6 4.Bg2 Bg7 5.O-O O-O 6.e4 e5 7.Nc3 Nc6 8.h3\n"
"1/2-1/2\n";

// two games consecutively in one file, the first of which has a parsing error (escaped quote 
// in 'Site' tag causes the tag to be treated as multi-line and not correctly terminate).
const char* test14 =
"[Event \"chT TD 93\0\"]\n"
"[Site \"POL-chT (Lubniewice) ;TD 93\v\"]\n"
"[Date \"1993.??.??\"]\n"
"[Round \"?\"]\n"
"[White \"Kaminski, Marcin\"]\n"
"[Black \"Murdzia, Piotr\"]\n"
"[Result \"1/2-1/2\"]\n"
"[ECO \"C86\"]\n"
"[WhiteElo \"2450\"]\n"
"[BlackElo \"2435\"]\n"
"[PlyCount \"41\"]\n"
"[EventDate \"1993.??.??\"]\n"
"\n"
"1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Qe2 b5 7. Bb3 d6 8. c3\n"
"O-O 9. d4 Nd7 10. a4 Bb7 11. Rd1 Re8 12. axb5 axb5 13. Rxa8 Qxa8 14. Qxb5 Ba6\n"
"15. Qd5 Nf6 16. Qxf7+ Kh8 17. Bd5 Nxd5 18. Qxd5 Be2 19. Re1 Bxf3 20. gxf3 exd4\n"
"21. cxd4 1/2-1/2\n"
"\n"
"[Event \"izt ;CBM 37\"]\n"
"[Site \"Biel izt ;CBM 37\"]\n"
"[Date \"1993.??.??\"]\n"
"[Round \"13\"]\n"
"[White \"Kamsky, Gata\"]\n"
"[Black \"Van der Sterren, Paul\"]\n"
"[Result \"1/2-1/2\"]\n"
"[ECO \"C86\"]\n"
"[WhiteElo \"2645\"]\n"
"[BlackElo \"2525\"]\n"
"[PlyCount \"19\"]\n"
"[EventDate \"1993.??.??\"]\n"
"\n"
"1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Qe2 b5 7. Bb3 d6 8. c3\n"
"O-O 9. d4 Bg4 10. Rd1 1/2-1/2\n";

// this is a game played by my great-grandfather, approximately 25 years before I was born. There is nothing remarkable about the syntax of this PGN.
//
// Jim Sarazin for The Ottawa Citizen, in its Fri Oct 20th 1967 issue, on page 5, writes about a 36-player simul exhibition held in the RA Centre by
// Boris Spassky. F. Krupilnicki, my great-grandfather, was the last of these players to be defeated by Spassky. If you have any info on any of the 
// games from this exhibition, contact me. I'll buy you a coffee.
const char *test15 =
"[Event \"Ottawa-Kingston m\"]\n"
"[Site \"Kingston CAN\"]\n"
"[Date \"1969.02.16\"]\n"
"[Round \"?\"]\n"
"[White \"Krupilnicki, F.\"]\n"
"[Black \"Stewart, Ken\"]\n"
"[Result \"1-0\"]\n"
"[ECO \"A71\"]\n"
"[WhiteFideId \"-1\"]\n"
"[WhiteFideId \"-1\"]\n"
"[PlyCount \"67\"]\n"
"[GameId \"2160767913118844\"]\n"
"[EventDate \"1969.02.16\"]\n"
"[EventType \"team-tourn\"]\n"
"[EventCountry \"CAN\"]\n"
"[SourceTitle \"Chess Chat 1969.04\"]\n"
"\n"
"1. d4 Nf6 2. c4 c5 3. d5 e6 4. Nc3 exd5 5. cxd5 d6 6. e4 g6 7. Nf3 Bg7 8. Bg5 O-O 9. Bd3 h6 10. Bh4 g5 11. Bg3 Nh5 12. Qd2 f5 13. exf5 Bxf5 14. Bxf5 Rxf5 15. O-O Nxg3 16. fxg3 Nd7 17. Qd3 Rf8 18. Rae1 Ne5 19. Nxe5 Bxe5 20. Qg6+ Bg7 21. Ne4 Qe7 22. Nxd6 Rxf1+ 23. Rxf1 Qe3+ 24. Kh1 Qe2 25. Qf7+ Kh8 26. Qf5 Rf8 27. Nf7+ Kg8 28. d6 Qxf1+ 29. Qxf1 Rxf7 30. Qb5 a6 31. Qe8+ Rf8 32. d7 Bf6 33. Qe6+ Kh8 34. h3 1-0\n";

static char *read_entire_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    size_t read;
    size_t written = 0;
    size_t chunks = 1;
    size_t chunk_size = 536870912; // ~0.5gb, == 2^29
    char *string = NULL;
    do {
        string = realloc(string, chunk_size * chunks);
        read = fread(string + (chunk_size * (chunks-1)), 1, chunk_size, f);
        if (read <= 0) break;
        written += read;
        chunks++;
    } while (1);
    string[written] = 0;
    fclose(f);
    *out_size = written;
    return string;
}

static void mpgn_print_slice(const MPGN_Slice s) {
    static char fmt[128];
    snprintf(fmt, sizeof(fmt)-4, "%%.%llds", s.count);
    printf(fmt, s.p);
}

// re-constructs the move's movetext from it's other properties, and then compares to the literal text in the file.
static int mpgn_validate_move(const MPGN_Move move) {
    char buf[32] = {0}; // longest possible move is probably something like: Nb5xd5+!?
    int buf_idx = 0;

    if (move.explicit_piece)                 buf[buf_idx++] = move.explicit_piece;
    if (move.source_file)                    buf[buf_idx++] = move.source_file;
    if (move.source_rank)                    buf[buf_idx++] = move.source_rank;
    if (move.flags & MPGN_Moveflag_Capture)  buf[buf_idx++] = 'x';
    if (move.target_file)                    buf[buf_idx++] = move.target_file;
    if (move.target_rank)                    buf[buf_idx++] = move.target_rank;
    if (move.flags & MPGN_Moveflag_Castle) {
        buf[buf_idx++] = 'O';
        buf[buf_idx++] = '-';
        buf[buf_idx++] = 'O';
        if (move.flags & MPGN_Moveflag_Queenside_Castle) {
            buf[buf_idx++] = '-';
            buf[buf_idx++] = 'O';
        }
    }
    if (move.promotion) {
        buf[buf_idx++] = '=';
        buf[buf_idx++] = move.promotion;
    }
    if (move.flags & MPGN_Moveflag_Mate) {
        buf[buf_idx++] = '#';
    } else if (move.flags & MPGN_Moveflag_Check) {
        buf[buf_idx++] = '+';
    }
    switch (move.csm) {
        case MPGN_CSM_Poormove:
            buf[buf_idx++] = '?';
            break;
        case MPGN_CSM_Dubious:
            buf[buf_idx++] = '?';
            buf[buf_idx++] = '!';
            break;
        case MPGN_CSM_Blunder:
            buf[buf_idx++] = '?';
            buf[buf_idx++] = '?';
            break;
        case MPGN_CSM_Goodmove:
            buf[buf_idx++] = '!';
            break;
        case MPGN_CSM_Interesting:
            buf[buf_idx++] = '!';
            buf[buf_idx++] = '?';
            break;
        case MPGN_CSM_Brilliant:  
            buf[buf_idx++] = '!';
            buf[buf_idx++] = '!';
            break;
    }
    if (move.flags & MPGN_Moveflag_Null) {
        if (move.text.p[0] == 'Z') {
            buf[buf_idx++] = 'Z';
            buf[buf_idx++] = '0';
        } else {
            buf[buf_idx++] = '-';
            buf[buf_idx++] = '-';
        }
    }

    int equal = mpgn_slice_eqn(move.text, buf, buf_idx);
    if (!equal) {
        printf("Failed to reconstruct the movetext for a move!\nWe constrcuted: %s\n      Original: ", buf);
        mpgn_print_slice(move.text);
        printf("\n");
    }
    return equal;
}

static int mpgn_run_test(const char *pgn) {
    char *test = (char*) pgn;
    MPGN_Node node;
    unsigned long long line            = 1;
    unsigned long long column          = 1;
    unsigned long long game_count      = 0;
    unsigned long long draw_count      = 0;
    unsigned long long white_win_count = 0;
    unsigned long long black_win_count = 0;
    unsigned long long unknown_result_count = 0;
    unsigned long long tag_count       = 0;
    unsigned long long comment_count   = 0;
    unsigned long long move_count      = 0;
    unsigned long long open_rav_count  = 0;
    unsigned long long close_rav_count = 0;
    unsigned long long nag_count       = 0;
    unsigned long long parsing_failure_count = 0;
    MPGN_Slice event  = {0};
    MPGN_Slice site   = {0};
    MPGN_Slice date   = {0};
    MPGN_Slice round  = {0};
    MPGN_Slice white  = {0};
    MPGN_Slice black  = {0};
    MPGN_Slice result = {0};
    while (mpgn_parse(&test, &node, &line, &column)) {
        switch (node.type) {
            case MPGN_Node_Tag:
                tag_count++;
                if (mpgn_slice_eq(node.tag.name, "Site")) {
                    site = node.tag.value;
                } else if (mpgn_slice_eq(node.tag.name, "Event")) {
                    event = node.tag.value;
                } else if (mpgn_slice_eq(node.tag.name, "Date")) {
                    date = node.tag.value;
                } else if (mpgn_slice_eq(node.tag.name, "White")) {
                    white = node.tag.value;
                } else if (mpgn_slice_eq(node.tag.name, "Black")) {
                    black = node.tag.value;
                } else if (mpgn_slice_eq(node.tag.name, "Round")) {
                    round = node.tag.value;
                } else if (mpgn_slice_eq(node.tag.name, "Result")) {
                    result = node.tag.value;
                }
                break;
            case MPGN_Node_Comment:
                comment_count++;
                break;
            case MPGN_Node_NAG:
                nag_count++;
                break;
            case MPGN_Node_Result:
                assert(open_rav_count == close_rav_count);
                memset((char*)&event,  sizeof(MPGN_Slice), 0);
                memset((char*)&site,   sizeof(MPGN_Slice), 0);
                memset((char*)&date,   sizeof(MPGN_Slice), 0);
                memset((char*)&round,  sizeof(MPGN_Slice), 0);
                memset((char*)&white,  sizeof(MPGN_Slice), 0);
                memset((char*)&black,  sizeof(MPGN_Slice), 0);
                memset((char*)&result, sizeof(MPGN_Slice), 0);
                switch (node.result.type) {
                    case MPGN_Result_White:
                        white_win_count++;
                        break;
                    case MPGN_Result_Black:
                        black_win_count++;
                        break;
                    case MPGN_Result_Draw:
                        draw_count++;
                        break;
                    case MPGN_Result_Unknown:
                        unknown_result_count++;
                        break;
                }
                break;
            case MPGN_Node_RAV_Begin:
                open_rav_count++;
                break;
            case MPGN_Node_RAV_End:
                close_rav_count++;
                break;
            case MPGN_Node_Move:
                move_count++;
                if (!mpgn_validate_move(node.move)) {
                    return 0;
                }
                break;
        }
    }

    if (node.type == MPGN_Node_Error) {
        printf("Event ");
        mpgn_print_slice(event);
        printf("\n");
        printf("Site ");
        mpgn_print_slice(site);
        printf("\n");
        printf("Date ");
        mpgn_print_slice(date);
        printf("\n");
        printf("Round ");
        mpgn_print_slice(round);
        printf("\n");
        printf("White ");
        mpgn_print_slice(white);
        printf("\n");
        printf("Black ");
        mpgn_print_slice(black);
        printf("\n");
        printf("Result ");
        mpgn_print_slice(result);
        printf("\n");
    }

    if (node.type == MPGN_Node_Error) {
        printf("\n%llu:%llu - %s\n", node.error.line, node.error.column, node.error.message);
        parsing_failure_count++;
        printf("Stopping parsing because we encountered an error.\n");
        return 0;
    }

    unsigned long long total_games = draw_count + white_win_count + black_win_count + unknown_result_count;
    printf("Parsed %llu games across %llu lines of text. %llu white wins, %llu black, %llu draws, %llu unknown results. %llu total tags, %llu comments, %llu total moves. RAV '(': %llu, ')': %llu, NAGs: %llu\n\n",
           total_games, line, white_win_count, black_win_count, draw_count, unknown_result_count, tag_count, comment_count, move_count, open_rav_count, close_rav_count, nag_count);
    if (parsing_failure_count) printf("Failed to parse: %llu games\n\n", parsing_failure_count);

    return 1;
}

static void mpgn_run_tests() {
    {
        const char *tests_that_should_pass[] = {
            "",
            test1,
            test2,
            test3,
            test4,
            test5,
            test6,
            test7,
            test8,
            test9,
            test10,
            test11,
            test12,
            test13,
            test15,
        };
        int passed_tests = 0;
        size_t num_tests = sizeof(tests_that_should_pass) / sizeof(char*);
        for (size_t i = 0; i < num_tests; i++) {
            printf("Running positive test #%zu\n", i + 1);
            if (!mpgn_run_test(tests_that_should_pass[i])) break;
            passed_tests += 1;
        }
        printf("----------------------------------------------------------------------\n%d / %zu tests (that should pass) passed\n\n\n", passed_tests, num_tests);
    }

    {
        const char *tests_that_should_fail[] = {
            test14,
        };
        int failed_tests = 0;
        size_t num_tests = sizeof(tests_that_should_fail) / sizeof(char*);
        for (size_t i = 0; i < num_tests; i++) {
            printf("Running negative test #%zu\n", i + 1);
            if (mpgn_run_test(tests_that_should_fail[i])) break;
            failed_tests += 1;
        }
        printf("----------------------------------------------------------------------\n%d / %zu tests (that should fail) failed\n", failed_tests, num_tests);
    }
}

int main(int argc, char **argv) {
    if (argc == 2) {
        printf("Going to test file: '%s'\n", argv[1]);
        size_t size = 0;
        char *file = read_entire_file(argv[1], &size);
#ifdef _WIN32
        LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
        LARGE_INTEGER Frequency;

        QueryPerformanceFrequency(&Frequency); 
        QueryPerformanceCounter(&StartingTime);
#endif
        mpgn_run_test(file);
#ifdef _WIN32
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
        printf("Took: %lluus. File size was %llu bytes. Throughput: %g gb/s\n", ElapsedMicroseconds.QuadPart, size, ((double)size/1000000000.0)/((double)ElapsedMicroseconds.QuadPart / 1000000.0));
#else
        printf("Finished parsing file '%s', %zu bytes in size\n", argv[1], size);
#endif
    } else {
        printf("Running default test suite because a test .pgn file path was not provided (or you passed more than one argument - either pass nothing or a single path to a file)...\n\n\n");
        mpgn_run_tests();
    }

    return 0;
}

/*
MIT License

Copyright 2026 Nick Hayashi

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
