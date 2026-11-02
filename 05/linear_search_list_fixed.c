/*
 * linear_search_list_fixed.c -- linear_search_list.c の修正版
 *
 * アルゴリズム第 1 第 5 回「探索 1：線形探索」
 * ダミー付き線形リストで表現した表に対する線形探索・挿入・削除
 * （石畑『アルゴリズムとデータ構造』§2.2 (c)(d) の Pascal プログラムを C に直したもの）
 *   search_previous ... プログラム 2.2.6（番兵を使った探索。見つけたセルの「直前」を返す）
 *                       中のコメントアウト部分を有効にするとプログラム 2.2.9（自己再構成リスト）
 *   insert_head     ... プログラム 2.2.7（表の先頭への挿入）
 *   erase_next      ... プログラム 2.2.8（表からの削除。直前のセルを使ってつなぎ替える）
 *
 * 元のファイル linear_search_list.c からの変更点
 *   1. cli_insert_head: 重複キーで拒否したとき、確保しておいた rec を free する
 *      （元は解放しておらず、拒否のたびに 1 セル分のメモリリークになっていた）
 *   2. search_previous: 自己再構成（コメントアウト部分）に previous = tab->header; を 1 行追加
 *      （先頭へ移した後の「直前」は header になる。これが無いと、有効にしたとき main が
 *        別のレコードを表示・削除する）
 *   3. insert_head のプロトタイプ宣言を search_previous の前に追加
 *      （2. を有効にしたとき、定義が後ろにある insert_head の暗黙宣言で警告／エラーにならないように）
 *   4. cli_insert_head の scanf を "%d %31s" にした（value[32] からのあふれを防ぐ。元は "%d %s"）
 *   5. コメントを追加した（動作は 1.〜4. 以外変えていない。実行結果も同じ）
 *
 * コンパイルと実行（Ubuntu 24.04 / gcc 13.3 で確認。警告 0）
 *   gcc -std=c11 -Wall -Wextra -Wpedantic linear_search_list_fixed.c
 *   ./a.out        途中で「キー 値」を 1 組入力する（例: 100 XXX）
 *
 * 教科書（Pascal）との対応
 *   header が先頭のダミー、sentinel が末尾のダミー（番兵）。教科書の p が current、p↑.next が current->next。
 *   new(p) / dispose(p) は malloc / free（init_record / free）。
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// レコード（リストのセル）: キー、それ以外の情報、次のセルへのポインタ
typedef struct record_ {
    int key;               // キー。同じ値は 2 回現れない（cli_insert_head が重複を拒否する）
    char value[32];        // キー以外のフィールド（31 文字 + 終端の '\0'）
    struct record_* next;  // 次のセル。最後の実レコードの next は sentinel
} record;

// 表: 先頭のダミー（header）と末尾のダミー（sentinel = 番兵）を持つ
//   header -> 実レコード -> ... -> 実レコード -> sentinel
//   header のおかげで、先頭への挿入・先頭の削除も途中と同じ手順で書ける（場合分けが減る）
//   sentinel のおかげで、探索が必ず止まる（配列版の records[length] と同じ役割）
typedef struct {
    record* header;
    record* sentinel;
} table;

// セルを 1 つ確保して key と value を入れる（教科書の new(p) に相当）。next は NULL
// value は 31 文字以内であること
record* init_record(int key, const char* value) {
    record* rec = (record*)malloc(sizeof(record));
    rec->next = NULL;
    rec->key = key;
    strcpy(rec->value, value);
    return rec;
}

// 表の全セルを解放する（header と sentinel も）。教科書には無い後始末
void clear(table* tab) {
    record* current = tab->header->next;
    while (current != tab->sentinel) {
        record* next = current->next;  // 解放する前に次のセルを覚えておく
        free(current);
        current = next;
    }

    free(tab->header);
    free(tab->sentinel);
    tab->header = NULL;
    tab->sentinel = NULL;
}

// 自己再構成（search_previous の中のコメントアウト部分）を有効にすると、
// 定義が後ろにある insert_head を search_previous から呼ぶので、先に宣言しておく
void insert_head(table* tab, record* rec);

// 線形探索（教科書プログラム 2.2.6、番兵つき）
// target と同じ key を持つセルを探し、その「直前のセル」を返す。見つからない場合は NULL を返す
// 直前を返すのは、削除（erase_next）でつなぎ替える元が必要だから。見つけたセル自体は 戻り値->next
// 計算量: O(n)（n は実レコードの個数）
record* search_previous(table* tab, int target) {
    tab->sentinel->key = target;         // 番兵: 末尾のダミーに target を書いておくと必ず止まる
    record* previous = tab->header;      // current の 1 つ後ろを付いてくる
    record* current = tab->header->next;
    while (target != current->key) {     // 比較は 1 回だけ
        previous = current;
        current = current->next;
    }
    bool found = current != tab->sentinel;  // 番兵で止まったら不成功

    // コメントアウトを外すと自己再構成リスト（教科書プログラム 2.2.9）になります。
    // 見つけたセルをリストから外して先頭へ差し込む（よく使うキーほど前に集まり、次の探索が速くなる）。
    // 最後の previous = tab->header; は、先頭へ移した後の current の直前が header になるので、
    // 戻り値（直前のセル）をそれに合わせるための行。
    // if (found) {
    //   previous->next = current->next;
    //   insert_head(tab, current);
    //   previous = tab->header;
    // }
    return found ? previous : NULL;
}

// 表の先頭への挿入（教科書プログラム 2.2.7）。探索しないので O(1)
// 2 重登録を防ぐなら、呼ぶ前に search_previous で調べる（cli_insert_head がその例）
void insert_head(table* tab, record* rec) {
    rec->next = tab->header->next;  // 1. 新しいセルの next を今の先頭へ
    tab->header->next = rec;        // 2. header の next を新しいセルへ（1. と 2. を逆にすると今の先頭を見失う）
}

// 表からの削除（教科書プログラム 2.2.8）。previous の次のセルを外して解放する。O(1)
// previous は search_previous の戻り値（NULL でないこと）。消す相手を見つける探索の分は別に O(n)
void erase_next(record* previous) {
    record* current = previous->next;
    previous->next = current->next;  // 先につなぎ替えてから
    free(current);                   // 解放する（逆にすると current->next が読めない）
}

// 標準入力から「キー 値」を 1 組読み、まだ無いキーなら先頭へ挿入する。既にあるキーなら読み直す
// 入力は数字で始まる前提（数字以外を入れたときの検査は省いている）
void cli_insert_head(table* tab) {
    printf("Type in a key (>= 0) and a field. (example: \"100 XX\")\n");
    while (true) {
        record* rec = init_record(-1, "");        // 先にセルを確保し、そこへ直接読み込む
        scanf("%d %31s", &rec->key, rec->value);  // %31s: value[32] からあふれないよう最大 31 文字で打ち切る（元は %s）

        if (search_previous(tab, rec->key) != NULL) {  // 2 重登録の検査（O(n) の探索が 1 回入る）
            printf("The key is already used.\n");
            free(rec);  // 使わなかったセルを解放する（元のファイルはここで解放しておらず、メモリリークになっていた）
        } else {
            insert_head(tab, rec);
            return;
        }
    }
}

// 表の内容を先頭から順に表示する（header と sentinel は表示しない）
void print(table* tab) {
    printf("TABLE: [ ");
    record* current = tab->header->next;
    while (current != tab->sentinel) {
        printf("{%d, %s} ", current->key, current->value);
        current = current->next;
    }
    printf("]\n");
}

int main() {
    // create table: header -> sentinel だけの空の表を作る（ダミーのキーは使わないので仮に -1）
    table tab;
    tab.sentinel = init_record(-1, "");
    tab.header = init_record(-1, "");
    tab.header->next = tab.sentinel;

    // 5 件を先頭へ挿入する（先頭挿入なので、表示は挿入した順の逆になる）
    insert_head(&tab, init_record(5, "EEE"));
    insert_head(&tab, init_record(2, "BBB"));
    insert_head(&tab, init_record(1, "AAA"));
    insert_head(&tab, init_record(3, "CCC"));
    insert_head(&tab, init_record(4, "DDD"));
    print(&tab);

    // insert user input: 標準入力から 1 件挿入する（例: 3 と入力すると "The key is already used." と出て読み直す）
    cli_insert_head(&tab);
    print(&tab);

    // search 3: キー 3 を探す（見つかれば previous->next が目的のセル）
    int target = 3;
    record* previous = search_previous(&tab, target);
    if (previous != NULL) {
        printf("%d was %s\n", target, previous->next->value);
    } else {
        printf("%d was NOT FOUND\n", target);
    }

    // erase 3: キー 3 を削除する（previous は上の search_previous の結果。見つかっている前提）
    erase_next(previous);
    print(&tab);

    // search 3: もう一度キー 3 を探す（今度は見つからず NULL）
    previous = search_previous(&tab, target);
    if (previous != NULL) {
        printf("%d was %s\n", target, previous->next->value);
    } else {
        printf("%d was NOT FOUND\n", target);
    }

    // 全セルを解放して終了
    clear(&tab);
    return 0;
}

// 実行結果
// TABLE: [ {4, DDD} {3, CCC} {1, AAA} {2, BBB} {5, EEE} ]
// Type in a key (>= 0) and a field. (example: "100 XX")
// 100 XXX
// TABLE: [ {100, XXX} {4, DDD} {3, CCC} {1, AAA} {2, BBB} {5, EEE} ]
// 3 was CCC
// TABLE: [ {100, XXX} {4, DDD} {1, AAA} {2, BBB} {5, EEE} ]
// 3 was NOT FOUND
