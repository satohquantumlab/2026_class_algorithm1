/*
 * binary_search_fixed.c -- binary_search.c の修正版
 *
 * アルゴリズム第 1 第 6 回「探索 2：二分探索，二分探索木」
 * 整列済みの配列で表現した表に対する二分探索・挿入・削除
 * （石畑『アルゴリズムとデータ構造』§2.3 の Pascal プログラムを C に直したもの）
 *   search ... プログラム 2.3.3（二分探索 方式 2。繰返し 1 回につき比較 1 回。「target 以下の最後の位置」を返す）
 *   insert ... プログラム 2.3.5（探索の結果を利用して正しい位置に挿入。後ろのレコードを 1 つずつずらす）
 *   erase  ... 線形探索の削除（プログラム 2.2.3）と同じ前詰め。原著 p.16「削除は線形探索の場合と同じプログラム」
 *
 * 元のファイル binary_search.c からの変更点
 *   1. erase に assert(0 <= pos && pos < tab->length) を追加
 *      （pos = -1（search の「無い」）や範囲外のまま呼ぶと records[-1] 等へ書き込むため。
 *        main では 3 が必ず表にあるので実行結果は変わらない）
 *   2. コメントを追加した（動作は 1. 以外変えていない。実行結果も同じ）
 *
 * コンパイルと実行（Ubuntu 24.04 / gcc 13.3 で確認。-std=c11 / -std=c2x とも警告 0、AddressSanitizer 異常なし）
 *   gcc -std=c11 -Wall -Wextra -Wpedantic binary_search_fixed.c
 *   ./a.out        入力は無い。rand() の種を変えていないので毎回同じ順で挿入され、同じ出力になる
 *
 * 教科書（Pascal）との対応
 *   添字は 0 から（Pascal は 1 から）。教科書の n は length、size は MAX_NUM_RECORDS、otherinfo は value。
 *   教科書の lo, hi, mid は low, high, middle。Pascal 版の「該当なし」hi = 0 は C 版では high = -1。
 *   見つかったかどうかは search の戻り値だけでは分からない（2.3.3 と同じ）。
 *   呼び出し側で records[index].key == target を確かめる（main の例）。
 */

#include <assert.h>
#include <stdbool.h>  // 元のファイルのまま（この版では bool を使っていない）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 表の容量（教科書の size）。二分探索には番兵が無いので MAX_NUM_RECORDS 個まで入る
#define MAX_NUM_RECORDS 1000

// レコード: キーと、それ以外の情報（教科書の otherinfo。ここでは文字列 1 つ）
typedef struct {
    int key;         // キー。表の中では昇順に並んでいる（insert がその位置に入れる）
    char value[32];  // キー以外のフィールド（31 文字 + 終端の '\0'）
} record;

// 表: 配列の前から length 個が実レコード。records[0..length-1] はキーの昇順
typedef struct {
    int length;
    record records[MAX_NUM_RECORDS];
} table;

// 二分探索（教科書プログラム 2.3.3、方式 2）
// key が target 以下となる最後の index を返す。そのような値が無い場合（target が全部より小さい、または表が空）は -1
// 繰返しごとに比較は 1 回。「等しい」場合は「大きい」場合と区別しない（else 側）ので、
// 一致した位置そのものを返す保証は無い。一致の確認は呼び出し側で行う
// ループ不変条件: records[0..low-1].key <= target < records[high+1..length-1].key
// 計算量: O(log n)（繰返し 1 回ごとに範囲が半分になる）
int search(table* tab, int target) {
    int low = 0;
    int high = tab->length - 1;
    while (low <= high) {
        int middle = (low + high) / 2;              // 教科書の (lo + hi) div 2
        if (target < tab->records[middle].key) {
            high = middle - 1;                      // middle 以降は target より大きい
        } else {
            low = middle + 1;                       // middle 以前は target 以下（等しい場合もこちら）
        }
    }
    return high;                                    // 教科書の pos := hi
}

// 整列を保つ挿入（教科書プログラム 2.3.5）。計算量 O(n)（位置を求めるのは O(log n)、ずらすのが O(n)）
// 2 重登録の検査はしていない（同じキーを入れると、既にある同じキーの後ろに入る）
void insert(table* tab, record rec) {
    assert(tab->length < MAX_NUM_RECORDS);  // 満杯なら中断（教科書の if n >= size then error）

    // 順序が正しく保たれるように、正しい挿入位置を計算します（rec.key 以下の最後の位置 index の直後に入れる）
    int index = search(tab, rec.key);

    // before: [0 1 2 4]
    // insert: 3 (index = 2)
    // after : [0 1 2   4]
    // 後ろから順に 1 つずつ右へずらす（前からずらすと値を上書きしてしまう。教科書 p.16 の注意）
    for (int i = tab->length; i > index + 1; i--) {
        tab->records[i] = tab->records[i - 1];
    }

    // before: [0 1 2   4]
    // after : [0 1 2 3 4]
    tab->records[index + 1] = rec;                  // 教科書の table[pos+1]
    tab->length++;
}

// 削除（線形探索の 2.2.3 と同じ前詰め）。pos 番目を消し、後ろを 1 つずつ前へ。計算量 O(n)
// pos は有効な添字（0 <= pos < length）であること。search の戻り値を渡すときは、一致を確かめてから
void erase(table* tab, int pos) {
    assert(0 <= pos && pos < tab->length);  // 追加: 範囲外（-1 など）のまま呼ぶと配列の外へ書き込むため

    // before: [0 1 2 3 4 5]
    // erase: 3 (index = 3)
    // after : [0 1 2 4 5]
    for (int i = pos; i < tab->length - 1; i++) {
        tab->records[i] = tab->records[i + 1];
    }
    tab->length--;
}

// 表の内容を前から順に表示する
void print(table* tab) {
    printf("[ ");
    for (int i = 0; i < tab->length; i++) {
        printf("{%d, %s} ", tab->records[i].key, tab->records[i].value);
    }
    printf("]\n");
}

// 入力をシャッフルするために用意した本題とは関係ない関数です。
// （Fisher-Yates のシャッフル。rand() の種を変えないので毎回同じ並びになる）
void shuffle(int* array, int length) {
    int i = length;
    while (i > 1) {
        int j = rand() % i--;
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

int main() {
    // create inputs: キー 0..4 をシャッフルして「整列されていない」入力を作る
    int num_keys = 5;
    int keys[5];
    for (int i = 0; i < num_keys; i++) {
        keys[i] = i;
    }
    shuffle(keys, num_keys);

    // 1 件ずつ insert すると、表は常にキーの昇順に保たれる（挿入順には依存しない）
    table tab = {0};
    for (int i = 0; i < num_keys; i++) {
        record rec = {keys[i], "AAA"};
        insert(&tab, rec);
    }
    print(&tab);

    // search 3: 戻り値 index は「3 以下の最後の位置」。本当に 3 かどうかは key を見て確かめる
    int target = 3;
    int index = search(&tab, target);
    if (index != -1 && tab.records[index].key == target) {
        printf("%d was %s\n", target, tab.records[index].value);
    } else {
        printf("%d was NOT FOUND.\n", target);
    }

    // erase 3: 上で見つかった位置を消す（見つかっている前提。見つからなかったときに呼ぶなら index の検査が要る）
    printf("erase: %d\n", target);
    erase(&tab, index);
    print(&tab);

    // search 3: もう一度探す。index は「3 以下の最後の位置」= 2 の位置を指すが key が違うので NOT FOUND
    index = search(&tab, target);
    if (index != -1 && tab.records[index].key == target) {
        printf("%d was %s\n", target, tab.records[index].value);
    } else {
        printf("%d was NOT FOUND.\n", target);
    }

    return 0;
}

// 実行結果
// [ {0, AAA} {1, AAA} {2, AAA} {3, AAA} {4, AAA} ]
// 3 was AAA
// erase: 3
// [ {0, AAA} {1, AAA} {2, AAA} {4, AAA} ]
// 3 was NOT FOUND.
