/*
 * avl_tree_fixed.c -- avl_tree.c の修正版
 *
 * アルゴリズム第 1 第 7 回「探索 3：平衡木」
 * AVL 木で表現した表への挿入
 * （石畑『アルゴリズムとデータ構造』§2.5 の Pascal プログラムを C に直したもの）
 *   rebalance ... プログラム 2.5.7（高くなった頂点を場合 1〜3 に分け、場合 3 なら回転でつり合いを直す）
 *   insert    ... プログラム 2.5.6（空の部分木に達したら新しいノードを置き、戻りながら rebalance を呼ぶ）
 *   print     ... 原著には無い。右→根→左の順に出力して木を横倒しに表示する（間順 左→根→右 の逆）
 *   clear     ... 原著には無い後始末（全ノードを解放）。この版で追加
 *
 * 元のファイル avl_tree.c からの変更点
 *   1. rebalance の 2 重回転: 新しい根 C の balance を BALANCED に戻す 1 行を追加した
 *      （教科書プログラム 2.5.7 の C↑.balance := balanced に当たる。元はこの行が無く、C に回転前の値が残る。
 *        main の 6 件では木の形は同じだが、たとえば 1, 2, 3, 6, 7, 4, 5 の順に挿入すると、
 *        元のプログラムは AVL 木でない木（頂点 3 で左右の高さの差が 2）を作る）
 *   2. main のコメント: 7 の挿入は case 3b ではなく case 3c（2 重回転）
 *   3. insert のコメント: 「リーフに達したら挿入」を「空の部分木（NULL）に達したら挿入」に直した
 *   4. insert: malloc の失敗（NULL）を assert で止める（元は検査せず NULL を書きに行く）
 *   5. insert: strcpy を snprintf にして value[32] からのあふれを防ぐ（元は 32 文字以上の value で壊れる）
 *   6. rebalance の末尾: assert(false) の後に return false を置いた
 *      （元は -DNDEBUG でコンパイルすると戻り値なしで関数を抜ける。正しい AVL 木では到達しない）
 *   7. clear を追加し、main の最後で全ノードを解放する（元は解放せずに終了する）
 *   8. コメントを追加した（動作は 1. と 4.〜7. 以外変えていない。main の実行結果は元と同じ）
 *
 * コンパイルと実行（Ubuntu 24.04 / gcc 13.3 で確認。-std=c11 / -std=c2x とも警告 0、AddressSanitizer・LeakSanitizer 異常なし）
 *   gcc -std=c11 -Wall -Wextra -Wpedantic avl_tree_fixed.c
 *   ./a.out        入力は無い。5, 8, 9, 4, 6, 7 の順に挿入し、途中の木を 4 回表示する
 *
 * 教科書（Pascal）との対応
 *   教科書の tree（= ↑node）は node*、変数引数 var p : tree は node**（p、p_current）で表す。
 *   child : array[left..right] of tree は children[2]。LEFT = 0、RIGHT = 1 をそのまま添字に使う。
 *   変数引数 var grown : boolean（この部分木の高さが 1 増えたか）は、関数の戻り値（bool）で表す。
 *   教科書の rebalance は insert の中に入れ子で書かれ、p と grown を共有する。
 *   C では関数を入れ子にできないので、p は引数、grown は戻り値で受け渡す。
 *   new(p) は malloc、error は assert、nil は NULL。
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 方向（教科書の direction = (left, right, balanced)）。2 通りに使う
//   children[] の添字 ... LEFT = 0（左の子）、RIGHT = 1（右の子）
//   balance の値      ... LEFT = 左の部分木が 1 だけ高い、RIGHT = 右の部分木が 1 だけ高い、BALANCED = 左右同じ高さ
typedef enum {
    LEFT,
    RIGHT,
    BALANCED,
} direction;

// ノード（教科書の node）
typedef struct node_ {
    int key;                    // キー。左部分木のキーはすべてこれより小さく、右部分木のキーはすべて大きい
    char value[32];             // キー以外のフィールド（31 文字 + 終端の '\0'）
    struct node_* children[2];  // children[LEFT] が左の子、children[RIGHT] が右の子（無ければ NULL）
    direction balance;          // 左右の部分木の高さの差（高さそのものは持たない）
} node;

// つり合いの回復（教科書プログラム 2.5.7）。*p を根とする部分木で、inserted_dir 側の部分木が 1 段高くなった後に呼ぶ
// *p を A、A の inserted_dir 側の子を B、B の反対側の子を C と呼ぶ（教科書の図 2.5.3、図 2.5.5 の記号）
// inserted_dir を左、opposite_dir を右と読めば、教科書の図のままになる
// 部分木が成長した場合は true を、そうでない場合は false を返す。
bool rebalance(node** p, direction inserted_dir) {
    direction opposite_dir;  // 教科書の opp（inserted_dir の反対側）
    if (inserted_dir == LEFT) {
        opposite_dir = RIGHT;
    } else {
        opposite_dir = LEFT;
    }

    // case 1: 挿入された方向と逆の高さが1高い場合
    //   低かった側が伸びて左右が同じ高さになる。A の高さは変わらないので、親には伝えない
    node* a = *p;
    if (a->balance == opposite_dir) {
        a->balance = BALANCED;
        return false;
    }

    // case 2: 左右の高さが等しい場合
    //   挿入された側が 1 高くなる（AVL 木の条件の範囲内）。A の高さが 1 増えるので、親に伝える
    if (a->balance == BALANCED) {
        a->balance = inserted_dir;
        return true;
    }

    // case 3: 挿入された方向の高さが1高い場合
    //   高かった側がさらに伸びて、左右の差が 2 になる。回転で木を組み替える
    // case 3a: 1重回転（B も挿入された側が高い = 外側の部分木 α が伸びた。教科書の図 2.5.3）
    node* b = a->children[inserted_dir];
    if (b->balance == inserted_dir) {
        a->children[inserted_dir] = b->children[opposite_dir];  // βをBからAに付け替え
        b->children[opposite_dir] = a;                          // βの場所にAを入れる
        a->balance = BALANCED;                                  // 回転後は A も B も左右同じ高さ
        b->balance = BALANCED;
        *p = b;  // Bを上に持ち上げる
        return false;  // 回転後の高さは挿入前と同じ。親には伝えない
    }

    // case 3c: 2重回転（B は反対側が高い = 内側の部分木 β が伸びた。教科書の図 2.5.4、図 2.5.5）
    //   β の根を C とし、C の部分木を β1（inserted_dir 側）と β2（反対側）に分けて、C を一番上に持ち上げる
    if (b->balance == opposite_dir) {
        node* c = b->children[opposite_dir];
        b->children[opposite_dir] = c->children[inserted_dir];  // β1をCからBに付け替え
        a->children[inserted_dir] = c->children[opposite_dir];  // β2をCからAに付け替え
        c->children[inserted_dir] = b;                          // BをCに付け替え
        c->children[opposite_dir] = a;                          // AをCに付け替え
        // B と A の balance は、C のどちら側が高かったかで決まる（C の balance を書き換える前に調べる）
        //   β1 が高かった（C が inserted_dir）... B は左右同じ高さ、A は反対側が 1 高い
        //   β2 が高かった（C が opposite_dir）... B は挿入された側が 1 高い、A は左右同じ高さ
        //   C が BALANCED（C 自身が今挿入された頂点）... A も B も左右同じ高さ
        if (c->balance != opposite_dir) {
            b->balance = BALANCED;
        } else {
            b->balance = inserted_dir;
        }
        if (c->balance != inserted_dir) {
            a->balance = BALANCED;
        } else {
            a->balance = opposite_dir;
        }
        c->balance = BALANCED;  // 追加: 新しい根 C は左右同じ高さ（教科書の C↑.balance := balanced）
        *p = c;  // Cを上に持ち上げる
        return false;  // 回転後の高さは挿入前と同じ。親には伝えない
    }

    // case 3b (invalid)
    //   成長が A に伝わるのは B が case 2 を通ったときだけで、そのとき B の balance は LEFT か RIGHT。
    //   B が BALANCED のままここに来ることはない（教科書の「これはありえない」）
    assert(false);
    return false;  // 追加: -DNDEBUG でも戻り値を返す（到達しない）
}

// 挿入（教科書プログラム 2.5.6）。p_current は「挿入すべき部分木」を指すポインタのアドレス（教科書の var p）
// 二分探索木と同じように降りて空の部分木に新しいノードを置き、再帰から戻りながら、高くなった頂点で rebalance を呼ぶ
// false が返った時点で成長は止まっており、それより上の頂点では何もしない
// 部分木が成長した場合は true を、そうでない場合は false を返す。
bool insert(node** p_current, int key, const char* value) {
    node* current = *p_current;

    // 空の部分木（NULL）に達したら挿入
    if (current == NULL) {
        node* n = (node*)malloc(sizeof(node));
        assert(n != NULL);  // 追加: 確保に失敗したら中断（元は検査なし）
        n->key = key;
        n->children[LEFT] = NULL;
        n->children[RIGHT] = NULL;
        n->balance = BALANCED;
        snprintf(n->value, sizeof(n->value), "%s", value);  // 追加: 31 文字で打ち切る（元は strcpy）
        *p_current = n;  // NULL だった場所そのものを新しいノードで置き換える
        return true;     // 空だった部分木にノードが入ったので、必ず高くなる（教科書の grown := true）
    }

    // キーが一致したらエラー
    assert(key != current->key);  // 教科書の error（2 重登録）

    if (key < current->key) {
        if (insert(&current->children[LEFT], key, value)) {  // 左部分木に挿入し、それが高くなったら
            return rebalance(p_current, LEFT);               // この頂点でつり合いを調べる（教科書の if g then rebalance(left)）
        }
    } else {
        if (insert(&current->children[RIGHT], key, value)) {  // 右部分木に挿入し、それが高くなったら
            return rebalance(p_current, RIGHT);               // 教科書の if g then rebalance(right)
        }
    }
    return false;  // 下で成長が止まった。この頂点の高さは変わらない
}

// 木を横倒しにして表示する（教科書には無い）。右部分木→自分→左部分木の順で、深さ 1 につき 2 文字下げる
// 出力の上の行が右の子、下の行が左の子。「|」に近いほど根に近い
void print(node* current, int depth) {
    if (current == NULL) {
        return;
    }
    // right
    print(current->children[RIGHT], depth + 1);

    // current
    printf("|");
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("{%d, %s}\n", current->key, current->value);

    // left
    print(current->children[LEFT], depth + 1);
}

// 追加: 木の全ノードを解放する（教科書には無い後始末）。左・右を解放してから自分を解放し、ポインタを NULL に
void clear(node** p_current) {
    node* current = *p_current;
    if (current != NULL) {
        clear(&current->children[LEFT]);
        clear(&current->children[RIGHT]);
        free(current);
        *p_current = NULL;
    }
}

int main() {
    node* root = NULL;  // 空の木。&root を渡すのが教科書の insert(x, y, root, grown)

    insert(&root, 5, "5");
    insert(&root, 8, "8");  // case 2（5 は左右同じ高さだった → 右が 1 高い）
    printf("TREE:\n");
    print(root, 1);

    insert(&root, 9, "9");  // case 3a（5 は右が高く、その子 8 も右が高い → 1 重回転で 8 が根）
    printf("TREE:\n");
    print(root, 1);

    insert(&root, 4, "4");  // case 2（5 と 8 の両方。左が 1 高くなる）
    insert(&root, 6, "6");  // case 1（5 は左が高かった → 左右同じ高さ。8 には伝わらない）
    printf("TREE:\n");
    print(root, 1);

    insert(&root, 7, "7");  // case 3c（8 は左が高く、その子 5 は右が高い → 2 重回転で 6 が根）
    printf("TREE:\n");
    print(root, 1);

    clear(&root);  // 追加: 全ノードを解放
}

// 実行結果（元の avl_tree.c と同じ）
// TREE:
// |    {8, 8}
// |  {5, 5}
// TREE:
// |    {9, 9}
// |  {8, 8}
// |    {5, 5}
// TREE:
// |    {9, 9}
// |  {8, 8}
// |      {6, 6}
// |    {5, 5}
// |      {4, 4}
// TREE:
// |      {9, 9}
// |    {8, 8}
// |      {7, 7}
// |  {6, 6}
// |    {5, 5}
// |      {4, 4}
