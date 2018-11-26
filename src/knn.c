#include "knn.h"
#include "distancia.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

KnnDP knn_new (KnnLabel label, float *data)
{
    KnnDP dp = {label, data};
    return dp;
}

float knn_dist (
    KnnDP *p,
    KnnDP *q,
    unsigned int n,
    KnnDA da,
    float r
)
{
    switch (da)
    {
        case Euclidian:
            return distancia_euclidiana(n, p->data, q->data);
            break;

        case Chebyshev:
            return distancia_chebyshev(n, p->data, q->data);
            break;

        case Minkowski:
            return distancia_minkowski(n, p->data, q->data, r);
            break;
        default:
            exit(EXIT_FAILURE);
            return 0.0;
            break;
    }
}

void knn_measure_all (
    KnnDP *p,
    KnnDP *dataset,
    unsigned int dataset_s,
    unsigned int n,
    KnnDT *distances,
    KnnDA da,
    float r
)
{
    for (int i = 0; i < dataset_s; i++)
    {
        *(distances + i) = knn_dt_new(
            dataset + i,
            knn_dist(
                p,
                dataset + i,
                n,
                da,
                r
            )
        );
    }
}

int _knn_dt_cmp (const void *p, const void *q)
{
    KnnDT *_p = (KnnDT*)p;
    KnnDT *_q = (KnnDT*)q;

    if (_p->distance > _q->distance)
    {
        return 1;
    } else if (_p->distance == _q->distance)
    {
        return 0;
    } else
    {
        return -1;
    }
}

struct _knn_label_counter
{
    KnnLabel label;
    unsigned int count;
};

int _knn_lc_cmp_by_count (const void *a, const void *b)
{
    struct _knn_label_counter *_a = (struct _knn_label_counter*)a;
    struct _knn_label_counter *_b = (struct _knn_label_counter*)b;

    return (int)(_a->count) - (int)(_b->count);
}

int _knn_lc_cmp_by_label (const void *a, const void *b)
{
    struct _knn_label_counter *_a = (struct _knn_label_counter*)a;
    struct _knn_label_counter *_b = (struct _knn_label_counter*)b;

    if (_a->label > _b->label)
    {
        return 1;
    } else if (_a->label == _b->label)
    {
        return 0;
    } else
    {
        return -1;
    }
}

KnnLabel _knn_most_present_label(KnnDT *distances, unsigned int k)
{
    struct _knn_label_counter *labels = malloc(sizeof(struct _knn_label_counter) * k);
    unsigned int tie = 0;
    KnnLabel result;

    // Os contadores devem ser zerados pois o lixo na memória pode afetar o
    // comportamento deste algorítmo
    for (int i = 0; i < k; i++)
    {
        labels[i].count = 0;
    }

    // Inicializa com a primeira label
    labels[0].label = distances->q->label;

    for (int i = 1; i < k; i++)
    {
        // Encontrar se a label já foi listada
        for (int j = 0; j < k; j++)
        {
            if (labels[j].count)
            {
                // Se encontrar, incrementar o contador
                if ((distances + i)->q->label == labels[j].label)
                {
                    labels[j].count++;
                    break;
                }
            } else // Se não encontrar, adiciona à lista e inicializa o contador
            {
                labels[j].label = (distances + i)->q->label;
                labels[j].count = 1;
            }
        }
    }

    // Ordena a lista de contadores
    qsort(labels, k, sizeof(struct _knn_label_counter), _knn_lc_cmp_by_count);

    // Verifica se há empate
    for (int i = 1; i < k; i++)
    {
        if(labels[0].count == labels[i].count) tie++;
        else break;
    }

    // No caso de empate, encontrar a label de menor valor
    if (tie)
    {
        qsort(labels, tie, sizeof(struct _knn_label_counter), _knn_lc_cmp_by_label);
    }

    result = labels[0].label;

    free(labels);

    return result;
}

void knn_delete (KnnDP *dp)
{
    free(dp->data);
    dp->data = NULL;
}

KnnDT knn_dt_new(KnnDP *p, float distance)
{
    KnnDT dt;
    dt.q = p;
    dt.distance = distance;

    return dt;
}

KnnLabel knn_classify (
    KnnDP *p,
    KnnDP *dataset,
    unsigned int dataset_s,
    unsigned int n,
    unsigned int k,
    KnnDA da,
    float r
)
{
    KnnDT *distances = malloc(sizeof(KnnDT) * dataset_s);
    KnnLabel class;

    knn_measure_all(p, dataset, dataset_s, n, distances, da, r);

    qsort(distances, dataset_s, sizeof(KnnDT), _knn_dt_cmp);

    class = _knn_most_present_label(distances, (k < dataset_s ? k : dataset_s));

    free(distances);

    return class;
}

void knn_print (KnnDP *dp, unsigned int n)
{
    printf("%.0f: (", dp->label);
    for (int i = 0; i < (n - 1); i++)
    {
        printf("%.2f, ", dp->data[i]);
    }
    printf("%.2f)\n", dp->data[n - 1]);
}

int _knn_label_cmp (const void *a, const void *b)
{
    KnnLabel *_a = (KnnLabel *) a;
    KnnLabel *_b = (KnnLabel *) b;

    if (*_a == *_b) return 0;
    else if (*_a > *_b) return 1;
    else return -1;
}

KnnLL knn_get_labels (KnnDP *dataset, unsigned int dataset_s)
{
    KnnLabel *labels = malloc(sizeof(KnnLabel) * dataset_s);
    unsigned int counter = 0;
    int labels_eq;

    for (int i = 0; i < dataset_s; i++)
    {
        labels[i] = 0;
    }

    for (int i = 0; i < dataset_s; i++)
    {
        labels_eq = 0;
        // Encontrar se a label já foi listada
        for (int j = 0; j <= counter; j++)
        {
            // Se encontrar, avança para a próxima iteração
            if (dataset[i].label == labels[j])
            {
                labels_eq = 1;
            }
        }
        if (!labels_eq)
        {
            labels[counter] = dataset[i].label;
            counter++;
        }
    }

    labels = realloc(labels, sizeof(KnnLabel) * counter);

    qsort(labels, counter, sizeof(KnnLabel), _knn_label_cmp);
    KnnLL label_struct = {counter, labels};
    return label_struct;
}
