#include "NTL.hl"

float2 setSpriteOffset(unsigned int numberRows, int index) {

    int column;
    int row ;
    if (numberRows != 0) {
        column = index % numberRows;
        row = index / numberRows;
    }
    float2 offset;
    if (numberRows != 0) {
        offset.x = (float)column / numberRows;
        offset.y = (float)row / numberRows;
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    return offset;
}

void updateSpriteCoordInfo(
        __global ParticleData *particle,
        __global ParticleDataSprite *spriteData,
        unsigned int numberRows) {
    float lifeFactor = fmod(particle->age / particle->lifeTime, 1.f);
    int stageCount = numberRows * numberRows;
    float atlasProgression = lifeFactor * stageCount;
    int index1 = (int)floor(atlasProgression);
    int index2 = index1 < stageCount -1 ? index1 + 1 : index1;

    spriteData->blend = fmod(atlasProgression, 1);
    spriteData->offset1 = setSpriteOffset(numberRows, index1);
    spriteData->offset2 = setSpriteOffset(numberRows, index2);
}

__kernel void sprite(__global ParticleData *data,
                    __global ParticleDataSprite *spriteData,
                    unsigned int numberRows) {
    uint id = get_global_id(0);
    __global ParticleData *particle = &data[id];

    updateSpriteCoordInfo(particle, &spriteData[id], numberRows);
}

