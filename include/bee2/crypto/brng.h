/*
*******************************************************************************
\file brng.h
\brief STB 34.101.47 (brng): algorithms of pseudorandom number generation
\project bee2 [cryptographic library]
\author (C) Sergey Agievich [agievich@{bsu.by|gmail.com}]
\created 2013.01.31
\version 2014.10.20
\license This program is released under the GNU General Public License 
version 3. See Copyright Notices in bee2/info.h.
*******************************************************************************
*/

/*!
*******************************************************************************
\file brng.h
\brief Алгоритмы СТБ 34.101.47 (brng)
*******************************************************************************
*/

#ifndef __BEE2_BRNG_H
#define __BEE2_BRNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bee2/defs.h"

/*!
*******************************************************************************
\file brng.h

Реализованы алгоритмы генерации псевдослучайных чисел, определенные
в СТБ 34.101.47 (brng). При ссылках на алгоритмы, таблицы, другие объекты
подразумеваются разделы СТБ 34.101.47-2012, в которых эти объекты
определены.

Во всех случаях в качестве вспомогательного алгоритма хэширования
используется belt-hash (см. СТБ 345.101.31).

В СТБ 34.101.47 определен вспомогательный алгоритм ключезависимого
хэширования HMAC. Реализация этого алгоритма на основе belt-hash
определена в belt.h.

Основные алгоритмы объединяются в группы, которые определяют следующие
криптографические <i>механизмы</i>:
-	CTR ---  генерация в режиме счетчика;
-	HMAC ---  генерация в режиме HMAC.

В механизме CTR используется ключ из 32 октетов. В механизме HMAC используется
ключ произвольной длины. Рекомендуется использовать ключ из 32 октетов.

Каждый механизм реализуется связкой из двух функций. Эти функции используют 
общее состояние, указатель на которое передается в функции как дополнительный 
параметр. Имеются функция определения объема памяти для состояния (_keep). 
Память для состояния готовит вызывающая программа и она же отвечает за очистку 
памяти.

Состояние можно копировать как фрагмент памяти.

В связке обязательно имеется функция инициализации алгоритма (Start) и
одна или несколько функций генерации и получения служебных данных (StepX).

Логика суффиксов функций StepX:
-	R -- rand (сгенерировать);
-	G -- get (получить синхропосылку).

Функция типа StepR соответствует интерфейсу gen_i (см. defs.h).

Функции связки спроектированы как максимально простые и эффективные.
В частности, в этих функциях не проверяются входные данные.

Каждая связка покрывается высокоуровневой функцией, которая
обрабатывает все данные целиком. В высокоуровневых функциях есть
проверка входных данных.

\expect Общее состояние связки функций не изменяется вне этих функций.

\expect{ERR_BAD_INPUT} Все входные указатели действительны.

\pre Если не оговорено противное, то входные буферы функций связки 
не пересекаются.
*******************************************************************************
*/

/*
*******************************************************************************
Генерация в режиме счетчика (CTR, алгоритм 6.2.4)
*******************************************************************************
*/

/*!	\brief Длина состояния функций CTR

	Возвращается длина состояния (в октетах) функций генерации в режиме CTR.
	\return Длина состояния.
*/
size_t brngCTR_keep();

/*!	\brief Инициализация режима CTR

	По ключу theta и синхропосылке iv в state формируются структуры данных, 
	необходимые для генерации псевдослучайных чисел в режиме CTR. 
	\pre По адресу state зарезервировано brngCTR_keep() октетов.
	\warning При многократном вызове функции с одним и тем же ключом должны
	использоваться различные синхропосылки. При повторе синхропосылок могут
	быть повторно сгенерированы те же данные.
	\remark Разрешается передавать нулевой указатель iv. В этом случае будет 
	использоваться нулевая синхропосылка.
*/
void brngCTRStart(
	void* state,			/*!< [out] состояние */
	const octet theta[32],	/*!< [in] ключ */
	const octet iv[32]		/*!< [in] синхропосылка */
);

/*!	\brief Генерация фрагмента в режиме CTR

	В буфер [count]buf записываются октеты, полученные в результате
	псевдослучайной генерации в режиме CTR. При генерации используются
	структуры данных, развернутые в state.
	\expect brngCTRStart() < brngCTRStepR()*.
	\remark Данные в режиме CTR генерируются блоками по 32 октета.
	Реализована буферизация блоков и функцию можно вызвать с произвольным
	значением count. Если не все данные сгенерированного ранее блока
	израсходованы, то они будут возвращены в первую очередь.
	\remark Первоначальное содержимое buf используется для формирования
	дополнительного слова X алгоритма генерации (см. п. 6.2.2). Слово X
	разбивается на последовательные блоки из 32 октетов, при необходимости
	дополняясь нулевыми октетами.
	\remark Если работает буферизация и возвращаются данные сгенерированного
	ранее блока, то соответствующие октеты buf не используются
	для формирования слова X, эти октеты пропускаются.
*/
void brngCTRStepR(
	void* buf,			/*!< [in/out] дополн. / псевдослучайные данные */
	size_t count,		/*!< [in] число октетов buf */
	void* state			/*!< [in/out] состояние */
);

/*!	\brief Получение синхропосылки режима CTR

	Возвращается синхропосылка iv, установленная при вызове brngCTRStart()
	и измененная затем при последовательных вызовах brngCTRStepR().
	\expect (brngCTRStepR()* < brngCTRStepG())*.
	\remark Если сгенерировано полное число блоков, то полученная синхропосылка 
	будет отличаться от ранее использованных. Поэтому эту синхропосылку можно 
	задавать при повторном вызове функций связки с тем же ключом.
*/
void brngCTRStepG(
	octet iv[32],		/*!< [out] синхропосылка */
	void* state			/*!< [in/out] состояние */
);

/*!	\brief Генерация в режиме CTR

	В буфер [count]buf записываются псевдослучайные данные,  сгенерированные 
	в режиме CTR на ключе theta при использовании синхропосылки iv. 
	Дополнительно в iv возвращается обновленная синхропосылка, которую можно 
	использовать при повторном вызове функции с тем же ключом.
	\return ERR_OK, если данные успешно сгенерированы, и код ошибки
	в противном случае.
	\warning При многократном вызове функции с одним и тем же ключом должны
	использоваться различные синхропосылки. При повторе синхропосылок могут
	быть повторно сгенерированы те же данные.
	\remark Первоначальное содержимое buf используется для формирования
	дополнительного слова X алгоритма генерации (см. п. 6.2.2). Слово X
	разбивается на последовательные блоки из 32 октетов, при необходимости
	дополняясь нулевыми октетами.
*/
err_t brngCTRRand(
	void* buf,				/*!< [in/out] дополн. / псевдослучайные данные */
	size_t count,			/*!< [in] число октетов buf */
	const octet theta[32],	/*!< [in] ключ */
	octet iv[32]			/*!< [in/out] первонач. / обновл. синхропосылка */
);

/*
*******************************************************************************
Генерация в режиме HMAC (HMAC, алгоритм 6.3.4)
*******************************************************************************
*/

/*!	\brief Длина состояния функций HMAC

	Возвращается длина состояния (в октетах) функций генерации в режиме HMAC.
	\return Длина состояния.
*/
size_t brngHMAC_keep();

/*!	\brief Инициализация режима HMAC

	По ключу [theta_len]theta и синхропосылке [iv_len]iv октетов
	в state формируются структуры данных, необходимые для генерации 
	псевдослучайных чисел в режиме HMAC.
	\pre По адресу state зарезервировано brngHMAC_keep() октетов.
	\expect Буфер iv остается корректным и постоянным вплоть до завершения
	работы с механизмом.
	\warning При многократном вызове функции с одним и тем же ключом должны
	использоваться различные синхропосылки. При повторе синхропосылок будут
	повторно сгенерированы те же данные.
	\remark Рекомендуется использовать ключ из 32 октетов.
*/
void brngHMACStart(
	void* state,			/*!< [out] состояние */
	const octet theta[],	/*!< [in] ключ */
	size_t theta_len,		/*!< [in] длина ключа в октетах */
	const octet iv[],		/*!< [in] синхропосылка */
	size_t iv_len			/*!< [in] длина синхропосылки в октетах */
);

/*!	\brief Генерация в режиме HMAC

	В буфер [count]buf записываются октеты, полученные в результате
	псевдослучайной генерации в режиме HMAC. При генерации используются
	структуры данных, развернутые в state.
	\expect brngHMACStart() < brngHMACStepR()*.
	\remark Данные в режиме HMAC генерируются блоками по 32 октета.
	Реализована буферизация блоков и функцию можно вызвать с произвольным
	значением count. Если не все данные сгенерированного ранее блока
	израсходованы, то они будут возвращены в первую очередь.

*/
void brngHMACStepR(
	void* buf,			/*!< [out] псевдослучайные данные */
	size_t count,		/*!< [in] размер buf в октетах */
	void* state			/*!< [in/out] состояние */
);

/*!	\brief Генерация в режиме HMAC

	В буфер [count]buf записываются псевдослучайные данные, сгенерированные 
	в режиме CTR на ключе [theta_len]theta при использовании 
	синхропосылки [iv_len]iv.
	\expect{ERR_BAD_INPUT} Буферы buf и iv не пересекаются.
	\return ERR_OK, если данные успешно сгенерированы, и код ошибки
	в противном случае.
	\warning При многократном вызове функции с одним и тем же ключом должны
	использоваться различные синхропосылки. При повторе синхропосылок будут
	повторно сгенерированы те же данные.
	\remark Ограничений на iv_len нет (ср. с функцией brngHMACStart()).
	\remark Рекомендуется использовать ключ из 32 октетов.
*/
err_t brngHMACRand(
	void* buf,				/*!< [out] выходные данные */
	size_t count,			/*!< [in] число октетов buf */
	const octet theta[],	/*!< [in] ключ */
	size_t theta_len,		/*!< [in] длина ключа в октетах */
	const octet iv[],		/*!< [in] синхропосылка */
	size_t iv_len			/*!< [in] длина синхропосылки в октетах */
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __BEE2_BRNG_H */
