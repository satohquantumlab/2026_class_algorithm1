/*
 * linear_search_array_fixed.c -- linear_search_array.c の修正版
 *
 * アルゴリズム第 1 第 5 回「探索 1：線形探索」
 * 配列で表現した表に対する線形探索・挿入・削除
 * （石畑『アルゴリズムとデータ構造』§2.2 (a)(b) の Pascal プログラムを C に直したもの）
 *   search ... プログラム 2.2.4（番兵を使った線形探索。2.2.1 の高速化版）
 *   insert ... プログラム 2.2.2（表の末尾への挿入。2 重登録の検査はしない）
 *   erase  ... プログラム 2.2.3（表からの削除。後ろのレコードを前へ詰める）
 *
 * 元のファイル linear_search_array.c からの変更点
 *   1. cli_insert の scanf を "%d %31s" にした（value[32] からのあふれを防ぐ。元は "%d %s"）
 *   2. コメントを追加した（動作は 1. 以外変えていない。実行結果も同じ）
 *
 * コンパイルと実行（Ubuntu 24.04 / gcc 13.3 で確認。警告 0）
 *   gcc -std=c11 -Wall -Wextra -Wpedantic linear_search_array_fixed.c
 *   ./a.out        途中で「キー 値」を 1 組入力する（例: 100 XXX）
 *
 * 教科書（Pascal）との対応
 *   添字は 0 から（Pascal は 1 から）。番兵の位置は records[length]（Pascal は table[n+1]）。
 *   教科書の n は length、size は MAX_NUM_RECORDS、otherinfo は value に当たる。
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 表の容量（教科書の size）。番兵のために 1 個使うので、入るレコードは最大 MAX_NUM_RECORDS - 1 個
#define MAX_NUM_RECORDS 1000

// レコード: キーと、それ以外の情報（教科書の otherinfo。ここでは文字列 1 つ）
typedef struct {
    int key;         // キー。同じ値は 2 回現れない（cli_insert が重複を拒否する）
    char value[32];  // キー以外のフィールド（31 文字 + 終端の '\0'）
} record;

// 表: 配列の前から length 個が実レコード。records[length] は番兵の席
typedef struct {
    int length;                       // 今入っているレコードの個数（教科書の n）
    record records[MAX_NUM_RECORDS];  // records[0] .. records[length-1] が実レコード
} table;

// 線形探索（教科書プログラム 2.2.4、番兵つき）
// target と同じ key を持つ record の index を返します。
// 条件に合致する record が見つからない場合は -1 を返します。
// 計算量: O(length)。不成功なら length 回、成功なら平均 length/2 回の比較
int search(table* tab, int target) {
    tab->records[tab->length].key = target;  // sentinel（番兵）: 末尾の次に target を置くと必ずどこかで一致して止まる
    int index = 0;
    while (target != tab->records[index].key) {  // 比較は 1 回だけ（2.2.1 は「範囲内か」と「一致したか」の 2 回）
        index++;
    }
    return index < tab->length ? index : -1;  // 番兵の位置（length）で止まったら不成功
}

// 表の末尾への挿入（教科書プログラム 2.2.2）。計算量: O(1)
// 2 重登録の検査はしない（必要なら呼ぶ前に search で調べる。cli_insert がその例）
void insert(table* tab, int key, const char* value) {
    assert(tab->length < MAX_NUM_RECORDS - 1);  // 番兵の席 records[length] を残すため、最大 MAX_NUM_RECORDS - 1 個まで

    tab->records[tab->length].key = key;
    strcpy(tab->records[tab->length].value, value);  // value は 31 文字以内であること（cli_insert は %31s で保証）
    tab->length++;
}

// 表からの削除（教科書プログラム 2.2.3）
// 添字 pos のレコードを消し、後ろのレコードを 1 つずつ前へ詰める。計算量: O(length)（平均 length/2 個の移動）
// pos は search で得た有効な添字（0 以上 length 未満）であること。-1（見つからなかったとき）のまま呼ばない
void erase(table* tab, int pos) {
    for (int i = pos; i < tab->length - 1; i++) {
        tab->records[i] = tab->records[i + 1];  // 構造体ごと代入（key と value をまとめてコピー）
    }
    tab->length--;
}

// 標準入力から「キー 値」を 1 組読み、まだ無いキーなら末尾へ挿入する。既にあるキーなら読み直す
// 入力は数字で始まる前提（数字以外を入れたときの検査は省いている）
void cli_insert(table* tab) {
    printf("Type in a key (>= 0) and a field. (example: \"100 XXX\")\n");
    while (true) {
        int key;
        char value[32];
        scanf("%d %31s", &key, value);  // %31s: value[32] からあふれないよう最大 31 文字で打ち切る（元は %s）
        if (search(tab, key) != -1) {   // 2 重登録の検査（O(length) の探索が 1 回入る）
            printf("The key is already used.\n");
        } else {
            insert(tab, key, value);
            return;
        }
    }
}

// 表の内容を先頭から順に表示する（番兵の席は表示しない）
void print(table* tab) {
    printf("TABLE: [ ");
    for (int i = 0; i < tab->length; i++) {
        printf("{%d, %s} ", tab->records[i].key, tab->records[i].value);
    }
    printf("]\n");
}

int main() {
    // create table: 空の表（length = 0）を作り、5 件を末尾へ挿入する（配列版は挿入した順に並ぶ）
    table tab = {0};
    insert(&tab, 5, "EEE");
    insert(&tab, 2, "BBB");
    insert(&tab, 1, "AAA");
    insert(&tab, 3, "CCC");
    insert(&tab, 4, "DDD");
    print(&tab);

    // insert user input: 標準入力から 1 件挿入する（例: 3 と入力すると "The key is already used." と出て読み直す）
    cli_insert(&tab);
    print(&tab);

    // search 3: キー 3 を探す（番兵つき線形探索）
    int target = 3;
    int index = search(&tab, target);
    if (index != -1) {
        printf("%d was %s\n", target, tab.records[index].value);
    } else {
        printf("%d was NOT FOUND\n", target);
    }

    // erase 3: キー 3 を削除する（index は上の search の結果。見つかっている前提）
    erase(&tab, index);
    print(&tab);

    // search 3: もう一度キー 3 を探す（今度は見つからず -1）
    index = search(&tab, target);
    if (index != -1) {
        printf("%d was %s\n", target, tab.records[index].value);
    } else {
        printf("%d was NOT FOUND\n", target);
    }

    return 0;
}

// 実行結果
// TABLE: [ {5, EEE} {2, BBB} {1, AAA} {3, CCC} {4, DDD} ]
// Type in a key (>= 0) and a field. (example: "100 XXX")
// 100 XXX
// TABLE: [ {5, EEE} {2, BBB} {1, AAA} {3, CCC} {4, DDD} {100, XXX} ]
// 3 was CCC
// TABLE: [ {5, EEE} {2, BBB} {1, AAA} {4, DDD} {100, XXX} ]
// 3 was NOT FOUND
