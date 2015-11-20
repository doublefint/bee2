/*
*******************************************************************************
\file botp.h
\brief STB 34.101.botp: experimental OTP algorithms
\project bee2 [cryptographic library]
\author (C) Sergey Agievich [agievich@{bsu.by|gmail.com}]
\created 2015.11.02
\version 2015.11.20
\license This program is released under the GNU General Public License 
version 3. See Copyright Notices in bee2/info.h.
*******************************************************************************
*/

/*!
*******************************************************************************
\file botp.h
\brief Алгоритмы управления одноразовыми паролями
*******************************************************************************
*/

#ifndef __BEE2_BOTP_H
#define __BEE2_BOTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bee2/defs.h"
#include "bee2/core/tm.h"

/*!
*******************************************************************************
\file botp.h

\section botp-common Общие положения

Реализованы алгоритмы управления одноразовыми паролями. Алгоритмы соответствуют
стандартам RFC 4226, RFC 6238, RFC 6287 и базируются на механизме имитозащиты 
HMAC[belt-hash], определенному в СТБ 34.101.47 и реализованному в модуле belt.

Пароль представляет собой строку из digit символов алфавита {'0' - '9'},
6 <= digit <= 8. 

Алгоритмы объединяются в группы, которые определяют следующие криптографические
механизмы (режимы):
-	HOTP ---  пароли на основе событий (RFC 4226);
-	TOTP ---  пароли на основе времени (RFC 6238);
-	OCRA ---  пароли на основе запросов (RFC 6287).

Каждый механизм реализуется связкой из нескольких функций. Эти функции 
используют общее состояние, указатель на которое передается в функции 
как дополнительный параметр. Имеются функция определения объема памяти 
для состояния (_keep). Память для состояния готовит вызывающая программа 
и она же отвечает за очистку памяти. Состояние можно копировать как фрагмент 
памяти.

В связке обязательно имеется функция инициализации режима (Start)
и одна или несколько функций обработки фрагментов данных и получения
результатов обработки (StepX).

Логика суффиксов функций StepX:
-	G -- generate (построить пароль);
-	V -- verify (проверить пароль, восстановить синхронизацию).

Функции связки спроектированы как максимально простые и эффективные.
В частности, в этих функциях не проверяются входные данные.

Каждая связка покрывается высокоуровневой функцией, которая
обрабатывает все данные целиком. В высокоуровневых функциях есть
проверка входных данных.

\expect Общее состояние связки функций не изменяется вне этих функций.

\expect{ERR_BAD_INPUT} Все входные указатели высокоуровневых функций 
действительны.

\pre Все входные указатели низкоуровневых функций действительны.
Размер буфера строки на 1 октет больше длины строки (с учетом
завершаюющего нулевого октета).

\pre Если не оговорено противное, то входные буферы функций связки 
не пересекаются.
*******************************************************************************
*/

/*!
*******************************************************************************
\file botp.h

\section botp-hotp Режим HOTP

Счетчик HOTP представляет собой строку из 8 октетов. Эта строка 
интерпретируются как число по правилам big-endian ("от старших к младшим"), 
принятым в RFC 4226. 

При выработке, а также при успешной проверке пароля счетчик инкрементируется. 
Обновленный счетчик может использоваться для генерации или проверки нового 
пароля. Инкремент выполняется по модулю 2^64.

При проверке пароля счетчик в худшем случае будет инкрементирован 
attempts + 1 раз. Ограничение на attempts введено для защиты от зацикливания 
и для контроля вероятности угадывания пароля.

\todo Уточнить ограничение на attempts. М.б. связать attemtps с digits.
*******************************************************************************
*/

/*!	\brief Длина состояния функций HOTP

	Возвращается длина состояния (в октетах) функций механизма HOTP.
	\return Длина состояния.
*/
size_t botpHOTP_keep();

/*!	\brief Инициализация режима HOTP

	По ключу [key_len]key в state формируются структуры данных, необходимые 
	для управления паролями в режиме HOTP.
	\pre По адресу state зарезервировано botpHOTP_keep() октетов.
	\remark Рекомендуется использовать ключ из 32 октетов.
*/
void botpHOTPStart(
	void* state,			/*!< [out] состояние */
	const octet key[],		/*!< [in] ключ */
	size_t key_len			/*!< [in] длина ключа в октетах */
);

/*!	\brief Генерация очередного пароля в режиме HOTP

	По ключу, размещенному в state, и счетчику ctr генерируется
	одноразовый пароль [digit]otp. После генерации счетчик инкрементируется.
	\pre 6 <= digit && digit <= 8.
	\expect botpHOTPStart() < botpHOTPStepG()*.
*/
void botpHOTPStepG(
	char* otp,			/*!< [in] одноразовый пароль */
	size_t digit,		/*!< [in] длина пароля */
	octet ctr[8],		/*!< [in/out] счетчик */
	void* state			/*!< [in/out] состояние */
);

/*!	\brief Проверка очередного пароля в режиме HOTP

	По ключу, размещенному в state, и счетчику ctr строится одноразовый 
	пароль из digit = strLen(otp) символов. Построенный пароль сравнивается
	с otp. Если пароли различаются, то счетчик инкрементируется, вычисляется
	и проверяется новый пароль. Процедура повторяется до тех пор, 
	пока не будет найдено совпадение или не будет выполнено attempts 
	дополнительных попыток. Если совпадение найдено, то в ctr будет возвращено 
	значение счетчика, следующего за успешным. Если совпадение не обнаружено, 
	то ctr не изменится.
	\pre otp состоит из десятичных символов.
	\pre 6 <= digit && digit <= 8.
	\pre attempts < 10.
	\expect botpHOTPStart() < botpHOTPStepV()*.
	\return Признак успеха.
*/
bool_t botpHOTPStepV(
	const char* otp,		/*!< [in] контрольный пароль */
	octet ctr[8],			/*!< [in/out] счетчик */
	size_t attempts,		/*!< [in] число попыток синхронизации */
	void* state				/*!< [in/out] состояние */
);

/*!	\brief Генерация пароля в режиме HOTP

	По ключу [key_len]key и счетчику ctr генерируется одноразовый пароль 
	[digit + 1]otp. После генерации счетчик инкрементируется.
	\expect{ERR_BAD_PARAMS} 6 <= digit && digit <= 8.
	\expect{ERR_BAD_INPUT} Буферы otp и ctr не пересекаются.
	\return ERR_OK, если пароль успешно сгенерирован, и код ошибки
	в противном случае.
*/
err_t botpHOTPGen(
	char* otp,			/*!< [in] одноразовый пароль */
	size_t digit,		/*!< [in] длина пароля */
	const octet key[],	/*!< [in] ключ */
	size_t key_len,		/*!< [in] длина ключа в октетах */
	octet ctr[8]		/*!< [in/out] счетчик */
);

/*!	\brief Проверка пароля в режиме HOTP

	По ключу [key_len]key и счетчику ctr строится одноразовый пароль 
	из digit = strLen(otp) символов. Построенный пароль сравнивается с otp. 
	Если пароли различаются, то счетчик инкрементируется, вычисляется
	и проверяется новый пароль. Процедура повторяется до тех пор, 
	пока не будет найдено совпадение или не будет выполнено attempts 
	дополнительных попыток. Если совпадение найдено, то в ctr будет возвращено 
	значение счетчика, следующего за успешным. Если совпадение не обнаружено, 
	то ctr не изменится.
	\expect{ERR_BAD_PWD} otp состоит из десятичных символов.
	\expect{ERR_BAD_PWD} 6 <= digit && digit <= 8.
	\expect{ERR_BAD_PARAMS} attempts < 10.
	\return ERR_OK, если пароль подошел, и код ошибки в противном случае.
*/
err_t botpHOTPVerify(
	const char* otp,		/*!< [in] контрольный пароль */
	const octet key[],		/*!< [in] ключ */
	size_t key_len,			/*!< [in] длина ключа в октетах */
	octet ctr[8],			/*!< [in/out] счетчик */
	size_t attempts			/*!< [in] число попыток синхронизации */
);


/*!
*******************************************************************************
\file botp.h

\section botp-totp Режим TOTP

Текущее время --- это, так называемое, UNIX-время --- число секунд, прошедших 
с момента 1970-01-01T00:00:00Z. Текущее время t округляется. Округление 
с параметрами t0, ts состоит в замене t <- (t - t0) / ts.
Здесь t0 -- базовая отметка времени, ts -- шаг времени. Округленную отметку t
можно получить с помощью функции tmTimeRound(). 

Рекомендуется использовать t0 = 0 и ts = 30 или 60.

Огругленная отметка времени интерпретируется как счетчик механизма HOTP, т.е.
как вычет по модулю 2^64. 

\todo Уточнить ограничения на attempts_fwd, attempts_bwd. 
М.б. связать attempts с digits и ts.
*******************************************************************************
*/

/*!	\brief Длина состояния функций TOTP

	Возвращается длина состояния (в октетах) функций механизма TOTP.
	\return Длина состояния.
*/
size_t botpTOTP_keep();

/*!	\brief Инициализация режима TOTP

	По ключу [key_len]key в state формируются структуры данных, необходимые 
	для управления паролями в режиме TOTP.
	\pre По адресу state зарезервировано botpTOTP_keep() октетов.
	\remark Рекомендуется использовать ключ из 32 октетов.
*/
void botpTOTPStart(
	void* state,			/*!< [out] состояние */
	const octet key[],		/*!< [in] ключ */
	size_t len				/*!< [in] длина ключа в октетах */
);

/*!	\brief Генерация очередного пароля в режиме TOTP

	По ключу, размещенному в state, и округленной отметке t текущего времени
	генерируется одноразовый пароль [digit + 1]otp.
	\pre 6 <= digit && digit <= 8.
	\pre t != TIME_MAX.
	\expect botpTOTPStart() < botpTOTPStepG()*.
*/
void botpTOTPStepG(
	char* otp,			/*!< [in] одноразовый пароль */
	size_t digit,		/*!< [in] длина пароля */
	tm_time_t t,		/*!< [in] округленная отметка времени */
	void* state			/*!< [in/out] состояние */
);

/*!	\brief Проверка очередного пароля в режиме TOTP

	По ключу, размещенному в state, и округленной отметке t текущего времени
	строится одноразовый пароль из strLen(otp) символов. Построенный пароль 
	сравнивается с otp. Если пароли различаются, то процедура повторяется 
	с другими отметками времени из интервала 
		{(t + i) % 2^64 : i = -attempts_bwd,..., attempts_fwd}
	до тех пор, пока совпадение паролей не будет обнаружено или весь интервал 
	не будет просмотрен.
	\pre otp состоит из десятичных символов.
	\pre 6 <= digit && digit <= 8.
	\pre t != TIME_MAX.
	\pre attempts_bwd < 5 && attempts_fwd < 5.
	\expect botpTOTPStart() < botpTOTPStepV()*.
*/
bool_t botpTOTPStepV(
	const char* otp,		/*!< [in] контрольный пароль */
	tm_time_t t,			/*!< [in] округленная метка времени */
	size_t attempts_bwd,	/*!< [in] число попыток "синхронизации назад" */
	size_t attempts_fwd,	/*!< [in] число попыток "синхронизации вперед" */
	void* state				/*!< [in/out] состояние */
);

/*!	\brief Генерация пароля в режиме TOTP

	По ключу [key_len]key и округленной отметке t текущего времени
	генерируется одноразовый пароль [digit + 1]otp.
	\expect{ERR_BAD_PARAMS} 6 <= digit && digit <= 8.
	\expect{ERR_BAD_PARAMS} t != TIME_MAX.
	\return ERR_OK, если пароль успешно сгенерирован, и код ошибки
	в противном случае.
*/
err_t botpTOTPGen(
	char* otp,			/*!< [in] одноразовый пароль */
	size_t digit,		/*!< [in] длина пароля */
	const octet key[],	/*!< [in] ключ */
	size_t key_len,		/*!< [in] длина ключа в октетах */
	tm_time_t t			/*!< [in] округленная отметка времени */
);

/*!	\brief Проверка пароля в режиме TOTP

	По ключу [key_len]key и округленной отметке t текущего времени
	строится одноразовый пароль из strLen(otp) символов. Построенный пароль 
	сравнивается с otp. Если пароли различаются, то процедура повторяется 
	с другими отметками времени из интервала 
		{(t + i) % 2^64 : i = -attempts_bwd,..., attempts_fwd}
	до тех пор, пока совпадение паролей не будет обнаружено или весь интервал 
	не будет просмотрен.
	\expect{ERR_BAD_PWD} otp состоит из десятичных символов.
	\expect{ERR_BAD_PWD} 6 <= digit && digit <= 8.
	\expect{ERR_BAD_PARAMS} attempts_bwd < 5 && attempts_fwd < 5.
	\expect{ERR_BAD_PARAMS} t != TIME_MAX.
	\return ERR_OK, если пароль подошел, и код ошибки в противном случае.
*/
err_t botpTOTPVerify(
	const char* otp,		/*!< [in] контрольный пароль */
	const octet key[],		/*!< [in] ключ */
	size_t key_len,			/*!< [in] длина ключа в октетах */
	tm_time_t t,			/*!< [in] округленная метка времени */
	size_t attempts_bwd,	/*!< [in] число попыток "синхронизации назад" */
	size_t attempts_fwd		/*!< [in] число попыток "синхронизации вперед" */
);

/*!
*******************************************************************************
\file botp.h

\section botp-ocra Режим OCRA

\todo
*******************************************************************************
*/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __BEE2_BOTP_H */