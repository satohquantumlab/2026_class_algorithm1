/*
 * binary_search_fixed.c -- 02/binary_search.c の修正版
 *
 * アルゴリズム第 1 第 2 回「アルゴリズムの計算量と O 記法」
 * 整列済みの整数配列に対する二分探索（実行時間の計測つき）。第 6 回の 06/binary_search.c の search と同じ方式 2
 * （石畑『アルゴリズムとデータ構造』プログラム 2.3.3 の考え方。key だけの配列なので table[high] と比べて一致を確かめる）
 *
 * 元のファイル binary_search.c からの変更点
 *   1. 入力の案内を「昇順で」に直した（二分探索は表が昇順に整列されていることが前提。第 2 回の点検で出した
 *      patch が新リポジトリに未反映だったもの）
 *   2. length の入力が数字でない・0 以下のときに終了する（元は malloc(負の長さ) や未初期化の配列アクセスになる）
 *   3. malloc の失敗（NULL）を検査する
 *   4. scanf の戻り値を見て、数字でない入力は終了扱いにする（元は読めなかったまま進む）
 *   5. clock() の戻り値を clock_t で受ける（元は long）
 *   6. コメントを追加した（出力の形式は変えていない）
 *
 * コンパイルと実行（Ubuntu 24.04 / gcc 13.3 で確認。-std=c11 / -std=c2x とも警告 0）
 *   gcc -std=c11 -Wall -Wextra -Wpedantic binary_search_fixed.c
 *   ./a.out        配列の長さ → 昇順の整数 → 探す整数（負の数で終了）の順に入力する
 *   大きな入力は input_64.txt などをリダイレクトで与える: ./a.out < input_1024.txt
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 二分探索（方式 2）: x 以下の最後の位置 high を求めてから、table[high] == x かどうかで一致を確かめる
// table は昇順に整列されていること。計算量は O(log n)
bool binary_search(int* table, int length, int x) {
    int low = 0;
    int high = length - 1;
    while (low <= high) {
        int middle = (low + high) / 2;
        if (x < table[middle]) {
            high = middle - 1;          // middle 以降は x より大きい
        } else {
            low = middle + 1;           // middle 以前は x 以下（等しい場合もこちら）
        }
    }
    return (high > -1) && (x == table[high]);  // high = -1 なら x は全要素より小さい
}

int main() {
    int length;
    printf("Type in the length of the array...\n");
    if (scanf("%d", &length) != 1 || length <= 0) {  // 追加: 数字でない・0 以下なら終了
        printf("The length must be a positive integer.\n");
        return 1;
    }

    int* table = (int*)malloc(length * sizeof(int));
    if (table == NULL) {                              // 追加: 確保に失敗したら終了
        printf("Out of memory.\n");
        return 1;
    }

    printf("Type in the table numbers in ascending order (number >= 0)...\n");  // 「昇順で」を明記（元は順序の案内なし）
    for (int i = 0; i < length; i++) {
        if (scanf("%d", &table[i]) != 1) {            // 追加: 数字でない入力は終了
            printf("Invalid input.\n");
            free(table);
            return 1;
        }
    }

    while (true) {
        int target = -1;
        printf("\nType in the target number... (Type in number < 0 to exit)\n");
        if (scanf("%d", &target) != 1 || target < 0) {  // 数字でない入力も終了扱い
            break;
        }

        clock_t start_clock = clock();
        bool found = binary_search(table, length, target);
        clock_t end_clock = clock();

        printf("CPU execution time: %.8lf s.\n", (double)(end_clock - start_clock) / CLOCKS_PER_SEC);
        printf("Is target in the table: %s\n", found ? "Yes" : "No");
    }

    free(table);

    return 0;
}

// 実行結果（実行時間は環境によって変わる）
// Type in the length of the array...
// 5
// Type in the table numbers in ascending order (number >= 0)...
// 1
// 3
// 3
// 5
// 9
//
// Type in the target number... (Type in number < 0 to exit)
// 3
// CPU execution time: 0.00000200 s.
// Is target in the table: Yes
//
// Type in the target number... (Type in number < 0 to exit)
// 5
// CPU execution time: 0.00000100 s.
// Is target in the table: Yes
//
// Type in the target number... (Type in number < 0 to exit)
// 8
// CPU execution time: 0.00000100 s.
// Is target in the table: No
//
// Type in the target number... (Type in number < 0 to exit)
// -1
