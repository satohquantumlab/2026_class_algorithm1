/*
 * binary_search_tree_fixed.c -- binary_search_tree.c の修正版
 *
 * アルゴリズム第 1 第 6 回「探索 2：二分探索，二分探索木」
 * 二分探索木で表現した表に対する探索・挿入・削除
 * （石畑『アルゴリズムとデータ構造』§2.4 の Pascal プログラムを C に直したもの）
 *   search      ... プログラム 2.4.3（再帰による探索。見つかればそのノード、無ければ NULL）
 *   insert      ... プログラム 2.4.4（空の部分木に達したら新しいノードを置く。var p は node** で表す）
 *   extract_max ... プログラム 2.4.9 の extractmax（部分木の最大ノードを取り外して返す）
 *   erase       ... プログラム 2.4.9 の delete（左の子が無ければ右の子をつなぐ。あれば左部分木の最大値を代役に）
 *   print       ... 原著には無い。右→根→左の順に出力して木を横倒しに表示する（間順 左→根→右 の逆）
 *   clear       ... 原著には無い後始末（全ノードを解放）
 *
 * 元のファイル binary_search_tree.c からの変更点
 *   1. init_node: malloc の失敗（NULL）を assert で止める（元は検査せず NULL を書きに行く）
 *   2. init_node: strcpy を snprintf にして value[32] からのあふれを防ぐ（元は 32 文字以上の value で壊れる）
 *   3. ファイル末尾の「実行結果」を実際の出力に差し替えた
 *      （元のコメントはキー 44, 55, 67, ... の別の例で、この main（キー 0〜14、target 8）の出力ではなかった）
 *   4. コメントを追加した（動作は 1.〜2. 以外変えていない。実行結果も同じ）
 *
 * コンパイルと実行（Ubuntu 24.04 / gcc 13.3 で確認。-std=c11 / -std=c2x とも警告 0、AddressSanitizer・LeakSanitizer 異常なし）
 *   gcc -std=c11 -Wall -Wextra -Wpedantic binary_search_tree_fixed.c
 *   ./a.out        入力は無い。rand() の種を変えていないので毎回同じ順で挿入され、同じ木・同じ出力になる
 *
 * 教科書（Pascal）との対応
 *   教科書の tree（= ↑node）は node*、変数引数 var p : tree は node**（p_current）で表す。
 *   *p_current への代入が、教科書の「nil だった部分木を新しい木で置き換える」に当たる。
 *   new(p) / dispose(p) は malloc / free。error は assert。nil は NULL。
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ノード（教科書の node）: キー、それ以外の情報、左右の部分木へのポインタ
typedef struct node_ {
    int key;              // キー。左部分木のキーはすべてこれより小さく、右部分木のキーはすべて大きい
    char value[32];       // キー以外のフィールド（31 文字 + 終端の '\0'）
    struct node_* left;   // 左の子（無ければ NULL）
    struct node_* right;  // 右の子（無ければ NULL）
} node;

// ノードを 1 つ確保して key と value を入れる（教科書の new(p) と with p↑ do ... に相当）。左右の子は NULL
node* init_node(int key, const char* value) {
    node* n = (node*)malloc(sizeof(node));
    assert(n != NULL);                               // 追加: 確保に失敗したら中断（元は検査なし）
    n->key = key;
    n->left = NULL;
    n->right = NULL;
    snprintf(n->value, sizeof(n->value), "%s", value);  // 追加: 31 文字で打ち切る（元は strcpy）
    return n;
}

// 木の全ノードを解放する（教科書には無い後始末）。左・右を解放してから自分を解放し、ポインタを NULL に
void clear(node** p_current) {
    node* current = *p_current;
    if (current != NULL) {
        clear(&current->left);
        clear(&current->right);
        free(current);
        *p_current = NULL;
    }
}

// 探索（教科書プログラム 2.4.3）。current を根とする部分木から target を探し、見つかればそのノード、無ければ NULL
// 小さければ左、大きければ右へ降りる。NULL に達したら「無い」。計算量は木の高さに比例（平均 O(log n)、最悪 O(n)）
node* search(node* current, int target) {
    if (current == NULL) {
        return NULL;                                 // 教科書の search := nil（見つからなかった）
    }

    if (target == current->key) {
        return current;                              // 教科書の search := p（見つかった）
    }

    if (target < current->key) {
        return search(current->left, target);        // 左部分木だけを探せばよい
    } else {
        return search(current->right, target);       // 右部分木だけを探せばよい
    }
}

// 挿入（教科書プログラム 2.4.4）。p_current は「挿入すべき部分木」を指すポインタのアドレス（教科書の var p）
// 空の部分木（*p_current == NULL）に達したら、そこに新しいノードを置く。同じキーがあれば中断（教科書の error = 2 重登録）
// 木の形は挿入の順序で決まる（親は子より先に挿入されたデータ）
void insert(node** p_current, int key, const char* value) {
    node* current = *p_current;
    if (current == NULL) {
        *p_current = init_node(key, value);          // nil だった場所そのものを新しいノードで置き換える
        return;
    }

    assert(key != current->key);                     // 2 重登録は中断（教科書の then error）

    if (key < current->key) {
        insert(&current->left, key, value);          // 教科書の insert(x, y, p↑.left)
    } else {
        insert(&current->right, key, value);         // 教科書の insert(x, y, p↑.right)
    }
}

// 部分木の最大ノードを取り外して返す（教科書プログラム 2.4.9 の extractmax。var q は node**）
// 右へ降り続け、右の子が無いノードが最大。取り外した跡には、そのノードの左の子をつなぐ
node* extract_max(node** p_current) {
    node* current = *p_current;
    if (current->right == NULL) {
        *p_current = current->left;                  // 教科書の q := q↑.left
        return current;                              // 教科書の extractmax := q
    }
    return extract_max(&current->right);             // 教科書の extractmax(q↑.right)
}

// 削除（教科書プログラム 2.4.9 の delete）。key を持つノードを探して取り除く。無ければ中断（教科書の error）
//   左の子が無い ... 右の子（NULL でも可）をそのままつなぐ（葉と、右の子だけを持つノードはこちら）
//   左の子がある ... 左部分木の最大ノード max を代役にし、元のノードの左右の子を max に付け替える
void erase(node** p_current, int key) {
    node* current = *p_current;
    assert(current != NULL);                         // 見つからなかった（教科書の if p = nil then error）

    if (key == current->key) {
        if (current->left == NULL) {
            *p_current = current->right;             // 教科書の p := p↑.right
            free(current);                           // 教科書の dispose(tmp)
        } else {
            node* max = extract_max(&current->left); // 教科書の r := extractmax(p↑.left)
            max->left = current->left;               // r↑.left := p↑.left（extract_max の後の左部分木）
            max->right = current->right;             // r↑.right := p↑.right
            *p_current = max;                        // p := r（代役に切替え）
            free(current);                           // dispose(p)
        }
        return;
    }

    if (key < current->key) {
        erase(&current->left, key);                  // 教科書の delete(x, p↑.left)
    } else {
        erase(&current->right, key);                 // 教科書の delete(x, p↑.right)
    }
}

// 木を横倒しにして表示する。右部分木→自分→左部分木の順（間順 左→根→右 の逆）で、深さ 1 につき 2 文字下げる
// 画面を左に 90 度回すと、上が右の子、下が左の子の木になる。全ノードを表示するだけで、探索とは別の処理
void print(node* current, int depth) {
    if (current == NULL) {
        return;
    }
    // right
    print(current->right, depth + 1);

    // current
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("{%d, %s}\n", current->key, current->value);

    // left
    print(current->left, depth + 1);
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
    // create inputs: キー 0..14 をシャッフルして挿入の順序を作る（順序が木の形を決める）
    int num_keys = 15;
    int keys[15];
    for (int i = 0; i < num_keys; i++) {
        keys[i] = i;
    }
    shuffle(keys, num_keys);

    // create tree: 空の木（root == NULL）に 1 件ずつ挿入する。&root を渡すのが教科書の insert(x, y, root)
    node* root = NULL;
    for (int i = 0; i < num_keys; i++) {
        insert(&root, keys[i], "AAA");
    }
    printf("TREE:\n");
    print(root, 0);

    // search target: 8 を探す（見つかればそのノード）
    int target = 8;
    node* result = search(root, target);
    if (result != NULL) {
        printf("%d is %s\n", target, result->value);
    } else {
        printf("%d is not found\n", target);
    }

    // erase target: 8 を削除する。この木では 8 の左の子が 7、右の子は無い → 左部分木の最大 7 が代役になる
    erase(&root, target);
    printf("%d was deleted.\n", target);
    printf("TREE:\n");
    print(root, 0);

    // search target: もう一度 8 を探す（今度は見つからない）
    result = search(root, target);
    if (result != NULL) {
        printf("%d is %s\n", target, result->value);
    } else {
        printf("%d is not found\n", target);
    }

    clear(&root);
    return 0;
}

// 実行結果（glibc の rand() の既定の種での挿入順。他の処理系では木の形が変わる）
// TREE:
//       {14, AAA}
//         {13, AAA}
//     {12, AAA}
//         {11, AAA}
//       {10, AAA}
//   {9, AAA}
//         {8, AAA}
//           {7, AAA}
//       {6, AAA}
//         {5, AAA}
//           {4, AAA}
//     {3, AAA}
//       {2, AAA}
//         {1, AAA}
// {0, AAA}
// 8 is AAA
// 8 was deleted.
// TREE:
//       {14, AAA}
//         {13, AAA}
//     {12, AAA}
//         {11, AAA}
//       {10, AAA}
//   {9, AAA}
//         {7, AAA}
//       {6, AAA}
//         {5, AAA}
//           {4, AAA}
//     {3, AAA}
//       {2, AAA}
//         {1, AAA}
// {0, AAA}
// 8 is not found
