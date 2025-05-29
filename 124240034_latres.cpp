#include <iostream>
#include <string>
#include <cstdio>  // untuk FILE*, fopen, fscanf, etc
#include <cstdlib> // untuk system("cls") dan system("pause")
using namespace std;

enum Status { TERSEDIA, DALAM_ANTREAN, SEDANG_DIPUTAR };
string statusToString(Status s) {
    return s == TERSEDIA ? "Tersedia" : s == DALAM_ANTREAN ? "Dalam Antrean" : "Sedang Diputar";
}

struct Video {
    string judul;
    int durasi;
    Status status;
    Video *left, *right;
    Video(string j, int d) : judul(j), durasi(d), status(TERSEDIA), left(NULL), right(NULL) {}
};

// Playlist Queue
struct QueueNode {
    Video* video;
    QueueNode* next;
    QueueNode(Video* v) : video(v), next(NULL) {}
};

struct Playlist {
    QueueNode* front = NULL;
    QueueNode* rear = NULL;

    bool isEmpty() { return front == NULL; }

    void enqueue(Video* v) {
        QueueNode* node = new QueueNode(v);
        if (rear) rear->next = node;
        else front = node;
        rear = node;
    }

    Video* dequeue() {
        if (isEmpty()) return NULL;
        QueueNode* temp = front;
        front = front->next;
        if (!front) rear = NULL;
        Video* v = temp->video;
        delete temp;
        return v;
    }

    void remove(Video* v) {
        QueueNode *prev = NULL, *curr = front;
        while (curr) {
            if (curr->video == v) {
                if (prev) prev->next = curr->next;
                else front = curr->next;
                if (curr == rear) rear = prev;
                delete curr;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
    }

    void resetFrontStatus() {
        if (front) front->video->status = SEDANG_DIPUTAR;
    }
};

// Stack Riwayat dan Undo
struct StackNode {
    string aksi;
    Video* video;
    StackNode* next;
    StackNode(string a, Video* v) : aksi(a), video(v), next(NULL) {}
};

struct Stack {
    StackNode* top = NULL;

    void push(string aksi, Video* v) {
        StackNode* node = new StackNode(aksi, v);
        node->next = top;
        top = node;
    }

    bool isEmpty() { return top == NULL; }

    StackNode* pop() {
        if (isEmpty()) return NULL;
        StackNode* temp = top;
        top = top->next;
        return temp;
    }

    void tampilkanRiwayat() {
        StackNode* curr = top;
        cout << "Riwayat:\n";
        while (curr) {
            cout << "- " << curr->video->judul << endl;
            curr = curr->next;
        }
    }

    void hapusTop() {
        if (!isEmpty()) {
            StackNode* temp = top;
            top = top->next;
            delete temp;
        }
    }
};

// Global
Video* root = NULL;
Playlist playlist;
Stack riwayat, aksiTerakhir;

// BST Tambah
Video* tambahVideo(Video* node, string judul, int durasi) {
    if (!node) {
        Video* v = new Video(judul, durasi);
        aksiTerakhir.push("TAMBAH", v);
        cout << "Video ditambahkan.\n";
        return v;
    }
    if (judul < node->judul) node->left = tambahVideo(node->left, judul, durasi);
    else if (judul > node->judul) node->right = tambahVideo(node->right, judul, durasi);
    else cout << "Judul sudah ada!\n";
    return node;
}

void inorder(Video* node) {
    if (!node) return;
    inorder(node->left);
    cout << node->judul << " (" << node->durasi << " mnt) - " << statusToString(node->status) << endl;
    inorder(node->right);
}

Video* cari(Video* node, string judul) {
    if (!node) return NULL;
    if (judul == node->judul) return node;
    if (judul < node->judul) return cari(node->left, judul);
    return cari(node->right, judul);
}

Video* tambahVideoLangsung(Video* node, Video* v) {
    if (!node) return v;
    if (v->judul < node->judul) node->left = tambahVideoLangsung(node->left, v);
    else if (v->judul > node->judul) node->right = tambahVideoLangsung(node->right, v);
    return node;
}

void tambahPlaylist(string judul) {
    Video* v = cari(root, judul);
    if (!v) {
        cout << "Video tidak ditemukan.\n";
        return;
    }
    if (v->status != TERSEDIA) {
        cout << "Video sudah diantre/diputar.\n";
        return;
    }
    v->status = playlist.isEmpty() ? SEDANG_DIPUTAR : DALAM_ANTREAN;
    playlist.enqueue(v);
    aksiTerakhir.push("PLAYLIST", v);
    cout << "Video masuk ke playlist.\n";
}

void simpanRiwayatKeFile() {
    FILE* file = fopen("riwayat.txt", "w");
    if (!file) {
        cout << "Gagal menyimpan riwayat ke file.\n";
        return;
    }

    StackNode* curr = riwayat.top;
    while (curr) {
        fprintf(file, "%s;%d\n", curr->video->judul.c_str(), curr->video->durasi);
        curr = curr->next;
    }

    fclose(file);
}

void muatRiwayatDariFile() {
    FILE* file = fopen("riwayat.txt", "r");
    if (!file) return;

    char judul[100];
    int durasi;

    while (fscanf(file, "%99[^;];%d\n", judul, &durasi) == 2) {
        // Cek apakah sudah ada di BST
        if (!cari(root, judul)) {
            // Tambah ke BST tanpa mencatat ke aksiTerakhir
            Video* vid = new Video(judul, durasi);
            root = tambahVideoLangsung(root, vid);  // fungsi bantu
            riwayat.push("RIWAYAT", vid);
        }
    }

    fclose(file);
}

void tontonVideo() {
    if (playlist.isEmpty()) {
        cout << "Playlist kosong.\n";
        return;
    }
    Video* v = playlist.dequeue();
    cout << "Menonton: " << v->judul << endl;
    v->status = TERSEDIA;
    riwayat.push("RIWAYAT", v);
    aksiTerakhir.push("TONTON", v);
    playlist.resetFrontStatus();

     simpanRiwayatKeFile();
}

void tampilkanRiwayat() {
    if (riwayat.isEmpty()) cout << "Riwayat kosong.\n";
    else riwayat.tampilkanRiwayat();
}

Video* hapusVideo(Video* node, string judul, bool confirm = true) {
    if (!node) return NULL;
    if (judul < node->judul) node->left = hapusVideo(node->left, judul, confirm);
    else if (judul > node->judul) node->right = hapusVideo(node->right, judul, confirm);
    else {
        if (node->status != TERSEDIA && confirm) {
            char y;
            cout << "Video sedang digunakan. Hapus? (y/t): "; cin >> y;
            if (y != 'y') return node;
        }
        playlist.remove(node);
        aksiTerakhir.push("HAPUS", node);
        if (!node->left) return node->right;
        else if (!node->right) return node->left;
        Video* succ = node->right;
        while (succ->left) succ = succ->left;
        node->judul = succ->judul;
        node->durasi = succ->durasi;
        node->status = succ->status;
        node->right = hapusVideo(node->right, succ->judul, false);
    }
    return node;
}

void undo() {
    if (aksiTerakhir.isEmpty()) {
        cout << "Tidak ada aksi untuk di-undo.\n";
        return;
    }
    StackNode* aksi = aksiTerakhir.pop();
    string tipe = aksi->aksi;
    Video* v = aksi->video;

    if (tipe == "TAMBAH") root = hapusVideo(root, v->judul, false);
    else if (tipe == "HAPUS") root = tambahVideo(root, v->judul, v->durasi);
    else if (tipe == "PLAYLIST") {
        playlist.remove(v);
        v->status = TERSEDIA;
    }
    else if (tipe == "TONTON") {
        riwayat.hapusTop();
        v->status = SEDANG_DIPUTAR;
        Playlist newQ;
        newQ.enqueue(v);
        while (!playlist.isEmpty()) newQ.enqueue(playlist.dequeue());
        playlist = newQ;
    }
    delete aksi;
    cout << "Undo berhasil.\n";
}

int main() {
    muatRiwayatDariFile();
    int pil;
    string judul;
    int durasi;

    do {
        system("cls"); // Membersihkan layar (khusus Windows)
        cout << "--- Menu IDLIX ---\n";
        cout << "1. Tambah Video\n2. Tampilkan Daftar\n3. Tambah ke Playlist\n";
        cout << "4. Tonton Video\n5. Riwayat\n6. Hapus Video\n7. Undo\n0. Keluar\n";
        cout << "Pilihan: "; cin >> pil; cin.ignore();

        system("cls");
        switch (pil) {
            case 1:
                cout << "Judul: "; getline(cin, judul);
                cout << "Durasi: "; cin >> durasi;
                root = tambahVideo(root, judul, durasi);
                break;
            case 2:
                inorder(root);
                cout << "Cari video? (y/t): "; char c; cin >> c; cin.ignore();
                if (c == 'y') {
                    cout << "Judul: "; getline(cin, judul);
                    Video* f = cari(root, judul);
                    if (f) cout << "Ditemukan: " << f->judul << " - " << statusToString(f->status) << endl;
                    else cout << "Tidak ditemukan.\n";
                }
                break;
            case 3:
                cout << "Judul: "; getline(cin, judul);
                tambahPlaylist(judul);
                break;
            case 4: tontonVideo(); break;
            case 5: tampilkanRiwayat(); break;
            case 6:
                cout << "Judul: "; getline(cin, judul);
                root = hapusVideo(root, judul);
                break;
            case 7: undo(); break;
        }

        if (pil != 0) {
            cout << endl;
            system("pause"); // Menunggu input sebelum lanjut
        }

    } while (pil != 0);
}
