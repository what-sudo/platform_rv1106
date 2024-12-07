#!/bin/bash

set -e

SCRITP_DIR=`realpath $0`
SCRITP_DIR=`dirname ${SCRITP_DIR}`
SDK_ROOT_DIR=${SCRITP_DIR}/../..
RELEASE_DIR=${SCRITP_DIR}/..
SDK_SYSDRV_DIR=${SDK_ROOT_DIR}/sysdrv

OUTPUT_DIR=${RELEASE_DIR}/build/output
BUILD_TMP_DIR=${RELEASE_DIR}/build/tmp

ROOTFS_SOURCE=${RELEASE_DIR}/fs/rootfs
ROOTFS_TMP_SOURCE=${BUILD_TMP_DIR}/rootfs
OEM_SOURCE=${RELEASE_DIR}/fs/oem
OEM_TMP_SOURCE=${BUILD_TMP_DIR}/oem
USERDATA_SOURCE=${RELEASE_DIR}/fs/userdata
IMAGES_SOURCE=${RELEASE_DIR}/images

ROOTFS_IMG="${OUTPUT_DIR}/rootfs.img"
OEM_IMG="${OUTPUT_DIR}/oem.img"
USERDATA_IMG="${OUTPUT_DIR}/userdata.img"

################################# 编译配置
CROSS_COMPILE=arm-rockchip830-linux-uclibcgnueabihf-

BUILDROOT_DIR=${SDK_ROOT_DIR}/sysdrv/source/buildroot
BUILDROOT_VER=buildroot-2023.02.6
BUILDROOT_DEFCONFIG=luckfox_pico_ultra_custom_defconfig

KERNEL_DIR=${SDK_ROOT_DIR}/sysdrv/source/kernel
KERNEL_DEFCONFIG=luckfox_rv1106_pico_ultra_custom_linux_defconfig
KERNEL_DTS=rv1106g-luckfox-pico-ultra-custom.dts

PARTITION="32K(env),512K@32K(idblock),256K(uboot),32M(boot),512M(rootfs),512M(oem),6G(userdata)"
ENV_PART_SIZE="0x8000"
BOOT_ENV="sys_bootargs= root=/dev/mmcblk0p5 rootfstype=erofs ro init=/linuxrc rk_dma_heap_cma=66M"

#################################

C_BLACK="\e[30;1m"
C_RED="\e[31;1m"
C_GREEN="\e[32;1m"
C_YELLOW="\e[33;1m"
C_BLUE="\e[34;1m"
C_PURPLE="\e[35;1m"
C_CYAN="\e[36;1m"
C_WHITE="\e[37;1m"
C_NORMAL="\033[0m"

rm -rf ${OUTPUT_DIR} ${BUILD_TMP_DIR}
mkdir -p ${OUTPUT_DIR}
mkdir -p ${BUILD_TMP_DIR}

function msg_info() {
	echo -e "${C_GREEN}[$(basename $0):info] $1${C_NORMAL}"
}

function finish_build() {
	msg_info "Running ${FUNCNAME[1]} succeeded."
	cd ${RELEASE_DIR}
}

function start_build() {
	msg_info "Start Running ${FUNCNAME[1]}."
}

function build_ext4() {

    # source files
    src=$1
    # generate image
    dst=$2

    dst_size=$3

    echo mkfs.ext4 -d $src -r 1 -N 0 -m 5 -L \"\" -O ^64bit,^huge_file $dst \"$dst_size\"

    mkfs.ext4 -d $src -r 1 -N 0 -m 5 -L "" -O ^64bit,^huge_file $dst "$dst_size"
    if [ $? != 0 ]; then
        echo "*** Maybe you need to increase the filesystem size "
        exit 1
    fi
    echo "resize2fs -M $dst"
    resize2fs -M $dst
    echo "e2fsck -fy  $dst"
    e2fsck -fy  $dst
    echo "tune2fs -m 5  $dst"
    tune2fs -m 5  $dst
    echo "resize2fs -M $dst"
    resize2fs -M $dst
}

function build_erofs() {
    # source files
    src=$1
    # generate image
    dst=$2

    mkfs.erofs -zlz4hc,12 ${dst} ${src}
}

function build_release() {
    start_build

    cp -rd ${ROOTFS_SOURCE} ${ROOTFS_TMP_SOURCE}
    cp -rd ${OEM_SOURCE} ${OEM_TMP_SOURCE}
    cp -rd ${RELEASE_DIR}/fs/overlay/* ${ROOTFS_TMP_SOURCE}

    cd ${BUILD_TMP_DIR}
    find ./ -type d | xargs -I {} rm -f {}/.gitkeep
    cd -

    build_erofs ${ROOTFS_TMP_SOURCE} ${ROOTFS_IMG}
    # build_ext4 ${ROOTFS_TMP_SOURCE} ${ROOTFS_IMG} 64M

    build_ext4 ${OEM_TMP_SOURCE} ${OEM_IMG} 64M
    build_ext4 ${USERDATA_SOURCE} ${USERDATA_IMG} 64M

    cp ${IMAGES_SOURCE}/* ${OUTPUT_DIR}/

    build_env

    finish_build
}

function buildroot_create() {
    start_build
    HCITOOL_TOOL_PATH=${SDK_ROOT_DIR}/sysdrv/tools/board/buildroot/hcitool_patch
    MPV_PATCH_PATH=${SDK_ROOT_DIR}/sysdrv/tools/board/buildroot/mpv_patch

    mkdir -p ${BUILDROOT_DIR}/${BUILDROOT_VER}
    tar xzf ${SDK_ROOT_DIR}/sysdrv/tools/board/buildroot/buildroot-2023.02.6.tar.gz -C ${BUILDROOT_DIR}
    cp -rfd ${SDK_ROOT_DIR}/sysdrv/buildroot_dl ${BUILDROOT_DIR}/${BUILDROOT_VER}/dl
    cp ${SDK_ROOT_DIR}/sysdrv/tools/board/buildroot/luckfox_pico_ultra_custom_defconfig ${BUILDROOT_DIR}/${BUILDROOT_VER}/configs/
    cp ${SDK_ROOT_DIR}/sysdrv/tools/board/buildroot/busybox-custom.config ${BUILDROOT_DIR}/${BUILDROOT_VER}/package/busybox/
    cp ${HCITOOL_TOOL_PATH}/0001-Fixed-header-file-errors.patch ${BUILDROOT_DIR}/${BUILDROOT_VER}/package/bluez5_utils/
    cp ${HCITOOL_TOOL_PATH}/0002-Fix-build-errors.patch ${BUILDROOT_DIR}/${BUILDROOT_VER}/package/bluez5_utils/
    cp ${HCITOOL_TOOL_PATH}/0003-fix-compat-wordexp.patch ${BUILDROOT_DIR}/${BUILDROOT_VER}/package/bluez5_utils/
    cp ${MPV_PATCH_PATH}/0002-change-j1.patch ${BUILDROOT_DIR}/${BUILDROOT_VER}/package/mpv/
    finish_build
}

function build_buildroot() {
    start_build

    [ -d ${BUILDROOT_DIR}/${BUILDROOT_VER} ] || {
        buildroot_create
    }

    make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} ${BUILDROOT_DEFCONFIG} -C ${BUILDROOT_DIR}/${BUILDROOT_VER}
    ${SDK_ROOT_DIR}/sysdrv/tools/board/mirror_select/buildroot_mirror_select.sh ${BUILDROOT_DIR}/${BUILDROOT_VER}/.config
    make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} source -C ${BUILDROOT_DIR}/${BUILDROOT_VER}
    make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j$(nproc) -C ${BUILDROOT_DIR}/${BUILDROOT_VER}

    rm -rf ${ROOTFS_SOURCE}
    mkdir ${ROOTFS_SOURCE}
    tar xf ${BUILDROOT_DIR}/${BUILDROOT_VER}/output/images/rootfs.tar -C ${ROOTFS_SOURCE}
    cp ${BUILDROOT_DIR}/${BUILDROOT_VER}/output/images/rootfs.tar ${BUILD_TMP_DIR}

    cd ${ROOTFS_SOURCE}
    find ./ -type d | xargs -I {} touch {}/.gitkeep
    cd -

    finish_build
}

function build_buildrootconfig() {
    start_build

    if [ -d "$BUILDROOT_PATH" ]; then
        msg_info "Buildroot has been created"
    else
        buildroot_create
    fi

    if [ ! -f ${BUILDROOT_DIR}/${BUILDROOT_VER}/.config ]; then
        make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} ${BUILDROOT_DEFCONFIG} -C ${BUILDROOT_DIR}/${BUILDROOT_VER}
    fi

    BUILDROOT_CONFIG_FILE_MD5=$(md5sum "${BUILDROOT_DIR}/${BUILDROOT_VER}/.config")
    make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} menuconfig -C ${BUILDROOT_DIR}/${BUILDROOT_VER}

    BUILDROOT_CONFIG_FILE_NEW_MD5=$(md5sum "${BUILDROOT_DIR}/${BUILDROOT_VER}/.config")
    if [ "$BUILDROOT_CONFIG_FILE_MD5" != "$BUILDROOT_CONFIG_FILE_NEW_MD5" ]; then
        msg_info "Buildroot Save Defconfig"
        make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} savedefconfig -C ${BUILDROOT_DIR}/${BUILDROOT_VER}
        cp ${BUILDROOT_DIR}/${BUILDROOT_VER}/configs/luckfox_pico_ultra_custom_defconfig ${SDK_ROOT_DIR}/sysdrv/tools/board/buildroot/
    fi

    finish_build
}

function build_kernel() {
    start_build

    if [ ! -f ${SDK_SYSDRV_DIR}/source/.kernel_patch ]; then
        echo "============Apply Kernel Patch============"
        cd ${SDK_ROOT_DIR}
        git apply --verbose ${SDK_SYSDRV_DIR}/tools/board/kernel/*.patch
        if [ $? -eq 0 ]; then
            msg_info "Patch applied successfully."
            cp ${SDK_SYSDRV_DIR}/tools/board/kernel/kernel-drivers-video-logo_linux_clut224.ppm ${KERNEL_DIR}/drivers/video/logo/
            touch ${SDK_SYSDRV_DIR}/source/.kernel_patch
        else
            msg_error "Failed to apply the patch."
        fi
    fi

    if [ ! -f ${KERNEL_DIR}/.config ]; then
        make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} ${KERNEL_DEFCONFIG}
    fi

    make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} ${KERNEL_DTS%.dts}.img BOOT_ITS=boot.its -j$(nproc)

    cp -fv ${KERNEL_DIR}/boot.img ${IMAGES_SOURCE}/boot.img

    finish_build
}

function build_kernelconfig() {
    start_build
    if [ ! -f ${KERNEL_DIR}/.config ]; then
        make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} ${KERNEL_DEFCONFIG}
    fi

    KERNEL_CONFIG_FILE_MD5=$(md5sum "${KERNEL_DIR}/.config")
    make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} menuconfig

    KERNEL_CONFIG_FILE_NEW_MD5=$(md5sum "${KERNEL_DIR}/.config")
    if [ "$KERNEL_CONFIG_FILE_MD5" != "$KERNEL_CONFIG_FILE_NEW_MD5" ]; then
        msg_info "KERNEL Save Defconfig"
        make -C ${KERNEL_DIR} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} savedefconfig
        cp ${KERNEL_DIR}/defconfig ${KERNEL_DIR}/arch/arm/configs/${KERNEL_DEFCONFIG}
    fi

    finish_build
}

function build_env() {
    start_build
    MKENVIMAGE=${SDK_SYSDRV_DIR}/tools/pc/uboot_tools/mkenvimage
    echo "blkdevparts=mmcblk0:${PARTITION}" > ${OUTPUT_DIR}/.env.txt
    echo ${BOOT_ENV} >> ${OUTPUT_DIR}/.env.txt
    ${MKENVIMAGE} -s $ENV_PART_SIZE -p 0x0 -o ${OUTPUT_DIR}/env.img ${OUTPUT_DIR}/.env.txt
    finish_build
}

function build_clean() {
    start_build

    if [ $1 == "kernel" ]; then
        if [ -f ${KERNEL_DIR}/.config ]; then
            make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} distclean -C ${KERNEL_DIR}
        fi
        if [ -f ${SDK_SYSDRV_DIR}/source/.kernel_patch ]; then
            git clean -df ${KERNEL_DIR}
            git checkout ${KERNEL_DIR}
            rm -rf ${SDK_SYSDRV_DIR}/source/.kernel_patch
        fi
    elif [ $1 == "buildroot" ]; then
        if [ -f ${BUILDROOT_DIR}/${BUILDROOT_VER}/.config ]; then
            make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} distclean -C ${BUILDROOT_DIR}/${BUILDROOT_VER}
        fi
    fi

    rm -rf ${OUTPUT_DIR}
    finish_build
}

num=$#
option=""
while [ $# -ne 0 ]; do
	case $1 in
	clean)
		option="build_clean $2"
		break
		;;
	kernel) option=build_kernel ;;
	kernelconfig) option=build_kernelconfig ;;
	buildroot) option=build_buildroot ;;
	buildrootconfig) option=build_buildrootconfig ;;
	env) option=build_env ;;
	release) option=build_release ;;
	*) option=usage ;;
	esac
	if [ $((num)) -gt 0 ]; then
		shift
	fi
done

eval "${option:-build_release}"

# make ARCH=arm CROSS_COMPILE=arm-rockchip830-linux-uclibcgnueabihf-
