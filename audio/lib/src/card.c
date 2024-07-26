/* card.c - Sound card SoC device driver */

/*
 * The source code in this file refers to or is derived from GPL-licensed
 * source code. It is strictly proprietary and must NOT be distributed
 * externally under any circumstances.
 */

/*
DESCRIPTION
This file provides a card driver that manages card register.
*/

/* includes */

#include <vxWorks.h>
#include <iosLib.h>
#include <sysLib.h>
#include <soc.h>
#include <errno.h>
#include <hwif/vxbus/vxbLib.h>
#include <private/semLibP.h>
#include <vxSoundCore.h>

#undef DEBUG_VX_SND_SOC
#ifdef DEBUG_VX_SND_SOC
#include <private/kwriteLibP.h>    /* _func_kprintf */

#define VX_SND_SOC_DBG_OFF          0x00000000U
#define VX_SND_SOC_DBG_ERR          0x00000001U
#define VX_SND_SOC_DBG_INFO         0x00000002U
#define VX_SND_SOC_DBG_VERBOSE      0x00000004U
#define VX_SND_SOC_DBG_ALL          0xffffffffU
uint32_t vxSndCardDbgMask = VX_SND_SOC_DBG_ALL;

#define VX_SND_SOC_MSG(mask, fmt, ...)                                  \
    do                                                                  \
        {                                                               \
        if ((vxSndCardDbgMask & (mask)))                                \
            {                                                           \
            if (_func_kprintf != NULL)                                  \
                {                                                       \
                (* _func_kprintf)("SND_SOC card: [%s,%d] "fmt, __func__,\
                                  __LINE__, ##__VA_ARGS__);             \
                }                                                       \
            }                                                           \
        }                                                               \
    while (0)

#define SND_SOC_MODE_INFO(...)                                          \
    do                                                                  \
        {                                                               \
        if (_func_kprintf != NULL)                                      \
            {                                                           \
            (* _func_kprintf)(__VA_ARGS__);                             \
            }                                                           \
        }                                                               \
    while (0)

#define SND_SOC_ERR(fmt, ...)    \
                  VX_SND_SOC_MSG(VX_SND_SOC_DBG_ERR,     fmt, ##__VA_ARGS__)
#define SND_SOC_INFO(fmt, ...)   \
                  VX_SND_SOC_MSG(VX_SND_SOC_DBG_INFO,    fmt, ##__VA_ARGS__)
#define SND_SOC_DBG(fmt, ...)    \
                  VX_SND_SOC_MSG(VX_SND_SOC_DBG_VERBOSE, fmt, ##__VA_ARGS__)

#else /* DEBUG_VX_SND_SOC */
#define SND_SOC_ERR(x...)      do { } while (FALSE)
#define SND_SOC_INFO(x...)     do { } while (FALSE)
#define SND_SOC_DBG(x...)      do { } while (FALSE)
#define SND_SOC_MODE_INFO(...) do { } while (FALSE)
#endif /* !DEBUG_VX_SND_SOC */

/* defines */

#define SNDRV_DEFAULT_IDX1  (-1)
#define SNDRV_DEFAULT_STR1  NULL

/* forward declarations */

LOCAL STATUS snd_soc_card_probe (VX_SND_SOC_CARD * pCard);
LOCAL STATUS snd_soc_card_late_probe (VX_SND_SOC_CARD * pCard);
LOCAL void snd_soc_remove_pcm_runtime (VX_SND_SOC_CARD * card,
                                       VX_SND_SOC_PCM_RUNTIME * rtd);
LOCAL STATUS snd_soc_bind_card (VX_SND_SOC_CARD * pCard);
LOCAL void soc_cleanup_card_resources (VX_SND_SOC_CARD * card);

LOCAL STATUS snd_soc_register_dais (VX_SND_SOC_COMPONENT * component,
                                    VX_SND_SOC_DAI_DRIVER * dai_drv,
                                    size_t count);
LOCAL void snd_soc_unregister_dais (VX_SND_SOC_COMPONENT * component);
LOCAL VX_SND_SOC_DAI * snd_soc_register_dai (VX_SND_SOC_COMPONENT *  component,
                                             VX_SND_SOC_DAI_DRIVER * dai_drv);
LOCAL STATUS snd_soc_add_pcm_runtime_unlock (VX_SND_SOC_CARD * pCard,
                                             VX_SND_SOC_DAI_LINK * daiLink);
LOCAL void snd_soc_rtd_add_component (VX_SND_SOC_PCM_RUNTIME * rtd,
                                      VX_SND_SOC_COMPONENT * component);
LOCAL BOOL snd_soc_is_matching_component (
                                      const VX_SND_SOC_DAI_LINK_COMPONENT * dlc,
                                      VX_SND_SOC_COMPONENT *component);
LOCAL VX_SND_SOC_PCM_RUNTIME * soc_new_pcm_runtime (VX_SND_SOC_CARD * card,
                                                 VX_SND_SOC_DAI_LINK * daiLink);
LOCAL STATUS soc_probe_link_components ( VX_SND_SOC_CARD * pCard);
LOCAL void soc_remove_component (VX_SND_SOC_COMPONENT * component, int probed);
LOCAL void soc_remove_link_components (VX_SND_SOC_CARD * pCard);
LOCAL void soc_remove_link_dais (VX_SND_SOC_CARD * pCard);
LOCAL void soc_set_name_prefix (VX_SND_SOC_COMPONENT * component);
LOCAL STATUS snd_soc_component_probe (VX_SND_SOC_COMPONENT * component);
LOCAL STATUS snd_soc_component_init (VX_SND_SOC_COMPONENT * component);
LOCAL STATUS soc_probe_component (VX_SND_SOC_CARD * card,
                                  VX_SND_SOC_COMPONENT * component);
LOCAL STATUS snd_soc_pcm_dai_probe (VX_SND_SOC_PCM_RUNTIME * rtd,
                                    int32_t order);
LOCAL STATUS soc_probe_link_dais (VX_SND_SOC_CARD * pCard);
LOCAL STATUS snd_soc_pcm_dai_new (VX_SND_SOC_PCM_RUNTIME * rtd);
LOCAL STATUS soc_init_pcm_runtime (VX_SND_SOC_CARD * pCard,
                                   VX_SND_SOC_PCM_RUNTIME * rtd);
LOCAL STATUS snd_soc_link_init (VX_SND_SOC_PCM_RUNTIME * rtd);
LOCAL STATUS snd_soc_runtime_set_dai_fmt (VX_SND_SOC_PCM_RUNTIME * rtd,
                                       uint32_t daiFmt);

/* locals */


LOCAL VX_SND_SOC_CARD * gDebugCard = NULL;

/* client_mutex is used to protect component_list */

LOCAL SEM_ID  client_mutex = SEM_ID_NULL;

LOCAL DL_LIST component_list =
    {
    .head = NULL,
    .tail = NULL,
    };

#define FOR_EACH_COMPONENT(cmpt)                                          \
    for (cmpt = (VX_SND_SOC_COMPONENT *) DLL_FIRST (&component_list);     \
         cmpt != NULL;                                                    \
         cmpt = (VX_SND_SOC_COMPONENT *) DLL_NEXT ((DL_NODE *) cmpt))

#define FOR_EACH_COMPONENT_DAIS(cmpt, dai)                                \
    for (dai = (VX_SND_SOC_DAI *) DLL_FIRST (&cmpt->dai_list);            \
         dai != NULL;                                                     \
         dai = (VX_SND_SOC_DAI *) DLL_NEXT ((DL_NODE *) dai))

void snd_soc_unregister_component
    (
    VXB_DEV_ID pDev
    )
    {
    VX_SND_SOC_COMPONENT * cmpt;
    BOOL                   findCmpt = FALSE;

    if (semTake (client_mutex, WAIT_FOREVER) != OK)
        {
        return;
        }

    FOR_EACH_COMPONENT (cmpt)
        {
        if (pDev == cmpt->pDev)
            {
            findCmpt = TRUE;
            break;
            }
        }

    if (findCmpt)
        {
        snd_soc_unregister_dais (cmpt);

        if (cmpt->card != NULL)
            {
            if (cmpt->card->instantiated == 1)
                {
                cmpt->card->instantiated = 0;
                soc_cleanup_card_resources (cmpt->card);
                }
            DLL_REMOVE (&component_list, &cmpt->cardNode);
            }
        }

    (void) semGive (client_mutex);
    }

STATUS snd_soc_unregister_card
    (
    VX_SND_SOC_CARD * card
    )
    {
    if (semTake (client_mutex, WAIT_FOREVER) != OK)
        {
        return ERROR;
        }

    if (card->instantiated == 1)
        {
        card->instantiated = 0;
        soc_cleanup_card_resources (card);
        }

    (void) semGive (client_mutex);

    SND_SOC_DBG ("Unregistered card '%s'\n", card->name);

    return OK;
    }

LOCAL STATUS snd_soc_card_probe
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    if (pCard->probe != NULL)
        {
        if (pCard->probe (pCard) != OK)
            {
            SND_SOC_ERR ("soc card probe() error\n");
            return ERROR;
            }

        pCard->probed = 1;
        }

    return OK;
    }

LOCAL STATUS snd_soc_card_late_probe
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    if (pCard->late_probe != NULL)
        {
        if (pCard->late_probe (pCard) != OK)
            {
            return ERROR;
            }

        pCard->probed = 1;
        }

    return OK;
    }

LOCAL void snd_soc_remove_pcm_runtime
    (
    VX_SND_SOC_CARD *        card,
    VX_SND_SOC_PCM_RUNTIME * rtd
    )
    {
    //to do: not affect normal work for now.
    }

LOCAL STATUS snd_soc_bind_card
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd;
    VX_SND_SOC_DAI_LINK *    dai_link;
    int32_t                  i;

    if (semTake (client_mutex, WAIT_FOREVER) != OK)
        {
        return ERROR;
        }

    if (semTake (pCard->cardMutex, WAIT_FOREVER) != OK)
        {
        (void) semGive (client_mutex);
        return ERROR;
        }

    /* add predefined DAI links to the list */

    pCard->num_rtd = 0;
    FOR_EACH_CARD_PRELINKS (pCard, i, dai_link)
        {
        if (snd_soc_add_pcm_runtime_unlock (pCard, dai_link) != OK)
            {
            SND_SOC_ERR ("failed to create runtime for dai link %s\n",
                         dai_link->name);
            goto probe_end;
            }
        }

    /* snd_card_new */

    if (snd_card_new (pCard->dev, &pCard->sndCard) != OK)
        {
        SND_SOC_ERR ("failed to create sound card\n");
        goto probe_end;
        }

    /* initialise the sound pCard only once */

    if (snd_soc_card_probe (pCard) != OK)
        {
        goto probe_end;
        }

    /* probe all components used by DAI links on this card */

    if (soc_probe_link_components (pCard) != OK)
        {
        SND_SOC_ERR ("failed to probe link components\n");
        goto probe_end;
        }

    /* probe all DAI links on this card */

    if (soc_probe_link_dais (pCard) != OK)
        {
        SND_SOC_ERR ("failed to probe link dais\n");
        goto probe_end;
        }

    FOR_EACH_SOC_CARD_RUNTIME (pCard, rtd)
        {
        if (soc_init_pcm_runtime (pCard, rtd) != OK)
            {
            SND_SOC_ERR ("failed to initialize soc pcm runtime %s\n",
                         rtd->daiLink->streamName);
            goto probe_end;
            }
        }

    if (vxSndSocCardCtrlsAdd (pCard, pCard->controls,
                              pCard->num_controls) != OK)
        {
        goto probe_end;
        }
    if (snd_soc_card_late_probe (pCard) != OK)
        {
        SND_SOC_ERR ("card late probe() error\n");
        goto probe_end;
        }

    if (snd_card_register (pCard->sndCard) != OK)
        {
        SND_SOC_ERR ("failed to register sound card\n");
        goto probe_end;
        }

    SND_SOC_INFO ("register sound card successfully\n");

    pCard->instantiated = 1;
    gDebugCard = pCard;

    (void) semGive (pCard->cardMutex);
    (void) semGive (client_mutex);

    return OK;

probe_end:

    soc_cleanup_card_resources (pCard);

    (void) semGive (pCard->cardMutex);
    (void) semGive (client_mutex);

    return ERROR;
    }

LOCAL void soc_cleanup_card_resources
    (
    VX_SND_SOC_CARD * card
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd;

    /* remove and free each DAI */

    soc_remove_link_dais (card);
    soc_remove_link_components (card);

    FOR_EACH_SOC_CARD_RUNTIME (card, rtd)
        {
        snd_soc_remove_pcm_runtime (card, rtd);
        }

    /* remove the card */

    if (card->probed != 0 && card->remove != NULL)
        {
        (void) card->remove (card);
        }

    if (card->sndCard != NULL)
        {
        snd_card_free(card->sndCard);
        card->sndCard = NULL;
        }
    }

// snd_soc_add_component
STATUS snd_soc_add_component
    (
    VX_SND_SOC_COMPONENT *  component,
    VX_SND_SOC_DAI_DRIVER * dai_drv,
    int32_t                 num_dai
    )
    {
    if (semTake (client_mutex, WAIT_FOREVER) != OK)
        {
        return ERROR;
        }

    if (snd_soc_register_dais (component, dai_drv, num_dai) != OK)
        {
        SND_SOC_ERR ("failed to register DAIs\n");
        goto err_cleanup;
        }

    /* see for_each_component */
    DLL_ADD (&component_list, &component->node);

    (void) semGive (client_mutex);

    return OK;

err_cleanup:

    (void) semGive (client_mutex);

    return ERROR;;
    }

LOCAL STATUS snd_soc_register_dais
    (
    VX_SND_SOC_COMPONENT *  component,
    VX_SND_SOC_DAI_DRIVER * dai_drv,
    size_t                  count
    )
    {
    VX_SND_SOC_DAI * dai;
    uint32_t         i;

    for (i = 0; i < count; i++)
        {
        dai = snd_soc_register_dai (component, dai_drv + i);
        if (dai == NULL)
            {
            snd_soc_unregister_dais (component);
            return ERROR;
            }
        }

    return OK;
    }

LOCAL void snd_soc_unregister_dais
    (
    VX_SND_SOC_COMPONENT *  component
    )
    {
    DL_NODE *pNode;

    while ((pNode = DLL_FIRST (&component->dai_list)) != NULL)
        {
        DLL_REMOVE (&component->dai_list, pNode);
        }
    }

LOCAL VX_SND_SOC_DAI * snd_soc_register_dai
    (
    VX_SND_SOC_COMPONENT *  component,
    VX_SND_SOC_DAI_DRIVER * dai_drv
    )
    {
    VXB_DEV_ID       pDev = component->pDev;
    VX_SND_SOC_DAI * dai;

    if (dai_drv->name == NULL)
        {
        SND_SOC_ERR ("dai_drv->name should not be NULL\n");
        return NULL;
        }

    dai = (VX_SND_SOC_DAI *) vxbMemAlloc (sizeof (VX_SND_SOC_DAI));
    if (dai == NULL)
        {
        SND_SOC_ERR ("failed to allocate dai memory for %s\n", dai_drv->name);
        return NULL;
        }

    /* deprecate legacy_dai_naming */

    dai->name = dai_drv->name;  /* Fix me: need to allocate new memory? */
    if (dai_drv->id != 0)
        {
        dai->id = dai_drv->id;
        }
    else
        dai->id = component->numDai;

    dai->component = component;
    dai->pDev      = pDev;
    dai->driver    = dai_drv;

    /* see for_each_component_dais */

    DLL_ADD (&component->dai_list, &dai->node);

    component->numDai++;

    return dai;
    }

//snd_soc_add_pcm_runtime  without client_mutex

LOCAL STATUS snd_soc_add_pcm_runtime_unlock
    (
    VX_SND_SOC_CARD     * pCard,
    VX_SND_SOC_DAI_LINK * daiLink
    )
    {
    VX_SND_SOC_PCM_RUNTIME *        rtd;
    VX_SND_SOC_DAI_LINK_COMPONENT * codec;
    VX_SND_SOC_DAI_LINK_COMPONENT * platform;
    VX_SND_SOC_DAI_LINK_COMPONENT * cpu;
    VX_SND_SOC_COMPONENT *          component;
    int32_t                         i;

    if (daiLink->ignore)
        {
        return OK;
        }

    SND_SOC_INFO ("binding %s\n", daiLink->name);

    rtd = soc_new_pcm_runtime (pCard, daiLink);
    if (rtd == NULL)
        {
        SND_SOC_ERR ("failed to create runtime for %s\n", daiLink->name);
        return ERROR;
        }

    /* Find CPU from registered CPUs */

    for (i = 0; i < daiLink->cpuNum != 0 && (cpu = &daiLink->cpus[i]) != NULL;
         i++)
        {
        rtd->dais[i] = snd_soc_find_dai (cpu);
        if (rtd->dais[i] == NULL)
            {
            SND_SOC_ERR ("cannot find DAI %s\n", cpu->dai_name);
            goto _err_defer;
            }
        snd_soc_rtd_add_component (rtd, rtd->dais[i]->component);
        }

    /* Find CODEC from registered CODECs */

    for (i = 0; i < daiLink->codecNum && (codec = &daiLink->codecs[i]) != NULL;
         i++)
        {
        rtd->dais[i + rtd->cpuNum] = snd_soc_find_dai (codec);
        if (rtd->dais[i + rtd->cpuNum] == NULL)
            {
            SND_SOC_ERR ("cannot find DAI %s\n", codec->dai_name);
            goto _err_defer;
            }
        snd_soc_rtd_add_component(rtd, rtd->dais[i + rtd->cpuNum]->component);
        }

    /* Find PLATFORM from registered PLATFORMs */

    for (i = 0; i < daiLink->platformNum &&
         (platform = &daiLink->platforms[i]) != NULL; i++)
        {
        FOR_EACH_COMPONENT(component)
            {
            if (!snd_soc_is_matching_component(platform, component))
                continue;
            snd_soc_rtd_add_component(rtd, component);
            }
        }

    return OK;

_err_defer:

    if (pCard->remove_dai_link)
        {
        pCard->remove_dai_link (pCard, rtd->daiLink);
        }

    DLL_REMOVE (&pCard->rtd_list, &rtd->node);

    /* to do, not need now: release component: snd_soc_pcm_component_free() */
    return ERROR;
    }

LOCAL void snd_soc_rtd_add_component
    (
    VX_SND_SOC_PCM_RUNTIME * rtd,
    VX_SND_SOC_COMPONENT   * component
    )
    {
    VX_SND_SOC_COMPONENT * comp;
    int32_t                i;

    FOR_EACH_RUNTIME_COMPONENTS (rtd, i, comp)
        {
        /* already connected */

        if (comp == component)
            return;
        }

    /* see for_each_rtd_components */

    rtd->components[rtd->componentNum] = component;
    rtd->componentNum++;
    }

LOCAL BOOL snd_soc_is_matching_component
    (
    const VX_SND_SOC_DAI_LINK_COMPONENT * dlc,
    VX_SND_SOC_COMPONENT * component
    )
    {
    if ((dlc == NULL) || (component == NULL))
        {
        SND_SOC_ERR ("dlc or component is NULL\n");
        return FALSE;
        }

    if (dlc->pDev != NULL && dlc->pDev == component->pDev)
        {
        return TRUE;
        }

    /*
     * dummy codec doesn't have pDev when machine drv set its dai link
     * component by using dummyDaiLinkCmpnt[], so use component name to
     * match the dummy codec
     */

    if (dlc->name && !strcmp (component->name, dlc->name))
        {
        return TRUE;
        }

    return FALSE;
    }

LOCAL VX_SND_SOC_PCM_RUNTIME * soc_new_pcm_runtime
    (
    VX_SND_SOC_CARD *      card,
    VX_SND_SOC_DAI_LINK *  daiLink
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd;
    VX_SND_SOC_COMPONENT *   component;

    /*
     * for rtd
     */

    rtd = vxbMemAlloc (sizeof(*rtd) + sizeof(component) *
                       (daiLink->cpuNum +
                        daiLink->codecNum +
                        daiLink->platformNum));
    if (rtd == NULL)
        {
        SND_SOC_ERR ("failed to allocate runtime memory for dai link %s\n",
                     daiLink->name);
        return NULL;
        }

    /*
     * for rtd->dais
     */

    rtd->dais = vxbMemAlloc (sizeof (VX_SND_SOC_DAI *) *
                             (daiLink->cpuNum + daiLink->codecNum));
    if (rtd->dais == NULL)
        {
        SND_SOC_ERR ("failed to allocate memory for rtd(%s)->dais\n",
                     daiLink->name);
        goto free_rtd;
        }

    /*
     * dais = [][][][][][][][][][][][][][][][][][]
     *    ^cpu_dais         ^codec_dais
     *    |--- num_cpus ---|--- num_codecs --|
     * see
     *  asoc_rtd_to_cpu()
     *  asoc_rtd_to_codec()
     */

    rtd->cpuNum   = daiLink->cpuNum;
    rtd->codecNum = daiLink->codecNum;
    rtd->card     = card;
    rtd->daiLink  = daiLink;
    rtd->num      = card->num_rtd++;

    /* see for_each_card_rtds */

    DLL_ADD (&card->rtd_list, &rtd->node);

    return rtd;

free_rtd:
    vxbMemFree(rtd);

    return NULL;
    }

VX_SND_SOC_DAI * snd_soc_find_dai
    (
    const VX_SND_SOC_DAI_LINK_COMPONENT * dlc
    )
    {
    VX_SND_SOC_COMPONENT * component;
    VX_SND_SOC_DAI *       dai;

//  lockdep_assert_held(&client_mutex);

    /* Find CPU DAI from registered DAIs */

    FOR_EACH_COMPONENT (component)
        {
        if (!snd_soc_is_matching_component (dlc, component))
            {
            continue;
            }

        FOR_EACH_COMPONENT_DAIS (component, dai)
            {
            if ((dlc->dai_name == NULL)||
                (dai->name == NULL) ||
                (strcmp(dai->name, dlc->dai_name)) ||
                (dai->driver == NULL) ||
                (dai->driver->name == NULL) ||
                (strcmp(dai->driver->name, dlc->dai_name)))
                {
                continue;
                }

            return dai;
            }
        }

    return NULL;
    }

LOCAL STATUS soc_probe_link_components
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    VX_SND_SOC_COMPONENT *   component;
    VX_SND_SOC_PCM_RUNTIME * rtd;
    int32_t                  i;
    int32_t                  order;

    for_each_comp_order (order)
        {
        FOR_EACH_SOC_CARD_RUNTIME (pCard, rtd)
            {
            FOR_EACH_RUNTIME_COMPONENTS (rtd, i, component)
                {
                if (component->driver->probe_order != order)
                    continue;

                if (soc_probe_component (pCard, component) != OK)
                    return ERROR;
                }
            }
        }

    return OK;
    }

LOCAL void soc_remove_component
    (
    VX_SND_SOC_COMPONENT * component,
    int                    probed
    )
    {
    if (!component->card)
        return;

    if (probed != 0)
        {
        if (component->driver->remove != NULL)
            {
            component->driver->remove (component);
            }
        }

    DLL_REMOVE (&component->card->component_dev_list, &component->cardNode);
    component->card = NULL;
    }

LOCAL void soc_remove_link_components
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    VX_SND_SOC_COMPONENT *   component;
    VX_SND_SOC_PCM_RUNTIME * rtd;
    int32_t                  i;
    int32_t                  order;

    for_each_comp_order (order)
        {
        FOR_EACH_SOC_CARD_RUNTIME (pCard, rtd)
            {
            FOR_EACH_RUNTIME_COMPONENTS (rtd, i, component)
                {
                if (component->driver->remove_order != order)
                    continue;
                soc_remove_component (component, 1);
                }
            }
        }
    }

LOCAL void soc_remove_link_dais
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd;
    int32_t                  order;
    int32_t                  i;
    VX_SND_SOC_DAI *         dai;

    for_each_comp_order (order)
        {
        FOR_EACH_SOC_CARD_RUNTIME (pCard, rtd)
            {
            FOR_EACH_RUNTIME_DAIS (rtd, i, dai)
                {
                if (dai->driver->remove_order != order)
                    continue;
                if (dai->probed && dai->driver->remove != NULL)
                    {
                    (void) dai->driver->remove (dai);
                    }
                dai->probed = 0;
                }
            }
        }
    }

LOCAL void soc_set_name_prefix
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    VXB_DEV_ID    pDev = component->pDev;
    VXB_FDT_DEV * pFdtDev;
    char *        prefix;
    int32_t       len;

    component->namePrefix = NULL;
    if (pDev == NULL)
        {
        SND_SOC_ERR ("component(%s) has no pDev\n", component->name);
        return;
        }

    pFdtDev = (VXB_FDT_DEV *) (vxbFdtDevGet (pDev));
    if (pFdtDev == NULL)
        {
        SND_SOC_ERR ("component(%s) has no pFdtDev\n", component->name);
        return;
        }

    prefix = (char *) vxFdtPropGet (pFdtDev->offset, "sound-name-prefix", &len);
    if (prefix == NULL)
        {
        SND_SOC_INFO ("component(%s) has no <sound-name-prefix>\n",
                      component->name);
        return;
        }

    SND_SOC_DBG ("component(%s)'s prefix is %s\n", component->name, prefix);

    component->namePrefix = prefix;
    }

LOCAL STATUS snd_soc_component_probe
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    if (component->driver->probe != NULL)
        {
        return component->driver->probe (component);
        }

    return OK;
    }

LOCAL STATUS snd_soc_component_init
    (
    VX_SND_SOC_COMPONENT * component
    )
    {
    if (component->init != NULL)
        {
        return component->init (component);
        }

    return OK;
    }


LOCAL STATUS soc_probe_component
    (
    VX_SND_SOC_CARD *      card,
    VX_SND_SOC_COMPONENT * component
    )
    {
    BOOL probed = FALSE;

    if (snd_soc_component_is_dummy (component))
        {
        return OK;
        }

    if (component->card != NULL)
        {
        if (component->card != card)
            {
            return ERROR;
            }
        return OK;
        }

    component->card = card;

    soc_set_name_prefix (component);

    if (snd_soc_component_probe(component) != OK)
        {
        SND_SOC_ERR ("component(%s) probe() error", component->name);
        goto err_probe;
        }

    probed = TRUE;
    if (snd_soc_component_init (component) != OK)
        {
        SND_SOC_ERR ("component(%s) init() error", component->name);
        goto err_probe;
        }

    if (vxSndSocComponentCtrlsAdd (component,
                                   (VX_SND_CONTROL *)component->driver->controls,
                                   component->driver->num_controls) != OK)
        {
        SND_SOC_ERR ("failed to add component(%s)'s controls", component->name);
        goto err_probe;
        }

    DLL_ADD (&card->component_dev_list, &component->cardNode);

    return OK;

err_probe:
    if (probed)
        {
        if (component->driver->remove != NULL)
            {
            component->driver->remove (component);
            }
        }
    component->card = NULL;

    /* may need to release other resource, not do now */

    return ERROR;
    }

LOCAL STATUS snd_soc_pcm_dai_probe
    (
    VX_SND_SOC_PCM_RUNTIME * rtd,
    int32_t                  order
    )
    {
    VX_SND_SOC_DAI * dai;
    int32_t          i;

    FOR_EACH_RUNTIME_DAIS (rtd, i, dai)
        {
        if (dai->driver->probe_order != order)
            continue;

        if (dai->driver->probe != NULL)
            {
            if (dai->driver->probe (dai) != OK)
                {
                return ERROR;
                }
            }

        dai->probed = 1;
        }

    return OK;
    }

//soc_probe_link_components
LOCAL STATUS soc_probe_link_dais
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    VX_SND_SOC_PCM_RUNTIME * rtd;
    int32_t                  order;

    for_each_comp_order (order)
        {
        FOR_EACH_SOC_CARD_RUNTIME (pCard, rtd)
            {
            SND_SOC_INFO ("probe %s dai link %d late %d\n",
                          pCard->name, rtd->num, order);

            /* probe all rtd connected DAIs in good order */

            if (snd_soc_pcm_dai_probe (rtd, order) != OK)
                {
                return ERROR;
                }
            }
        }

    return OK;
    }

LOCAL STATUS snd_soc_pcm_dai_new
    (
    VX_SND_SOC_PCM_RUNTIME * rtd
    )
    {
    VX_SND_SOC_DAI * dai;
    int              i;

    FOR_EACH_RUNTIME_DAIS (rtd, i, dai)
        {
        if (dai->driver->pcm_new != NULL)
            {
            if (dai->driver->pcm_new (rtd, dai) != OK)
                {
                return ERROR;
                }
            }
        }

    return OK;
    }

LOCAL STATUS soc_init_pcm_runtime
    (
    VX_SND_SOC_CARD *        pCard,
    VX_SND_SOC_PCM_RUNTIME * rtd
    )
    {
    VX_SND_SOC_DAI_LINK *  daiLink = rtd->daiLink;
    int32_t                num;

    /* do machine specific initialization */

    if (snd_soc_link_init (rtd) != OK)
        {
        return ERROR;
        }

    if (daiLink->dai_fmt != 0)
        {
        if (snd_soc_runtime_set_dai_fmt (rtd, daiLink->dai_fmt) != OK)
            {
            return ERROR;
            }
        }

    num = rtd->num;

    /* create the pcm */

    if (vxSndSocPcmCreate (rtd, num) != OK)
        {
        SND_SOC_ERR ("failed to create pcm for rtd %s\n",
                     rtd->daiLink->streamName);
        return ERROR;
        }

    return snd_soc_pcm_dai_new (rtd);
    }

//int snd_soc_link_init(struct snd_soc_pcm_runtime *rtd)

LOCAL STATUS snd_soc_link_init
    (
    VX_SND_SOC_PCM_RUNTIME * rtd
    )
    {
    if (rtd->daiLink->init != NULL)
        return rtd->daiLink->init (rtd);

    return OK;
    }

//int snd_soc_runtime_set_dai_fmt(struct snd_soc_pcm_runtime *rtd,
//                                unsigned int dai_fmt)
LOCAL STATUS snd_soc_runtime_set_dai_fmt
    (
    VX_SND_SOC_PCM_RUNTIME * rtd,
    uint32_t                 daiFmt
    )
    {
    VX_SND_SOC_DAI * cpuDai;
    VX_SND_SOC_DAI * codecDai;
    uint32_t         i;
    int32_t          ret;

    FOR_EACH_RUNTIME_CODEC_DAIS (rtd, i, codecDai)
        {
        ret = vxSndSocDaiFmtSet (codecDai, daiFmt);
        if (ret != 0 && ret != -ENOTSUP)
            {
            SND_SOC_ERR ("codec dai set format error\n");
            return ERROR;
            }
         }

    FOR_EACH_RUNTIME_CPU_DAIS (rtd, i, cpuDai)
        {
        ret = vxSndSocDaiFmtSet (cpuDai, daiFmt);
        if (ret != 0 && ret != -ENOTSUP)
            {
            SND_SOC_ERR ("cpu dai set format error\n");
            return ERROR;
            }
        }

    return OK;
    }

//snd_soc_of_get_dai_name
STATUS sndSocGetDaiName
    (
    VXB_DEV_ID              pDev,
    uint32_t                daiIdx,
    const char **           dai_name
    )
    {
    DL_NODE *               pDaiNode;
    VX_SND_SOC_COMPONENT *  pCompt;
    VX_SND_SOC_DAI *        pDai;
    uint32_t                i;

    if (semTake (client_mutex, WAIT_FOREVER) != OK)
        {
        SND_SOC_ERR ("failed to take client_mutex\n");
        return ERROR;
        }

    FOR_EACH_COMPONENT (pCompt)
        {
        if (pCompt->pDev == pDev && pCompt->numDai != 0)
            {
            if ((daiIdx + 1) > pCompt->numDai)
                {
                SND_SOC_ERR ("invalid DAI index:%d\n", daiIdx);
                (void) semGive (client_mutex);
                return ERROR;
                }

            pDaiNode = DLL_FIRST (&pCompt->dai_list);
            for (i = 1; i < daiIdx + 1; i++)
                {
                pDaiNode = DLL_NEXT (pDaiNode);
                }

            if (pDaiNode == NULL)
                {
                SND_SOC_ERR ("invalid DAI node\n");
                (void) semGive (client_mutex);
                return ERROR;
                }

            pDai = CONTAINER_OF (pDaiNode, VX_SND_SOC_DAI, node);

            *dai_name = pDai->driver->name;

            (void) semGive (client_mutex);
            return OK;
            }
        }

    (void) semGive (client_mutex);
    return ERROR;
    }

//snd_soc_register_component
STATUS vxSndCpntRegister
    (
    VXB_DEV_ID                  pDev,
    const VX_SND_SOC_CMPT_DRV * cpntDriver,
    VX_SND_SOC_DAI_DRIVER *     daiDrv,
    int32_t                     daiNum
    )
    {
    VX_SND_SOC_COMPONENT * cpnt;

    if (client_mutex == SEM_ID_NULL)
        {
        client_mutex = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
                                   SEM_INVERSION_SAFE);
        if (client_mutex == SEM_ID_NULL)
            {
            SND_SOC_ERR ("failed to create client_mutex\n");
            return ERROR;
            }
        }

    cpnt = (VX_SND_SOC_COMPONENT *) vxbMemAlloc (sizeof(*cpnt));
    if (cpnt == NULL)
        {
        SND_SOC_ERR ("failed to allocate component memory\n");
        return ERROR;
        }

    DLL_INIT (&cpnt->dai_list);

    cpnt->ioMutex = semMCreate (SEM_Q_PRIORITY
                                | SEM_DELETE_SAFE
                                | SEM_INVERSION_SAFE);

    cpnt->name = vxbMemAlloc (SND_DEV_NAME_LEN);
    if (cpnt->name == NULL)
        {
        SND_SOC_ERR ("failed to allocate cpnt->name memory\n");
        return ERROR;
        }

    if (cpntDriver->name != NULL)
        {
        (void) memcpy_s (cpnt->name, SND_DEV_NAME_LEN, cpntDriver->name,
                         SND_DEV_NAME_LEN);
        }
    else
        {
        (void) memcpy_s (cpnt->name, SND_DEV_NAME_LEN, vxbDevNameGet (pDev),
                         MAX_DEV_NAME_LEN);
        }

    cpnt->pDev = pDev;
    cpnt->driver = cpntDriver;

    return snd_soc_add_component (cpnt, daiDrv, daiNum);
    }

//snd_soc_register_card
STATUS vxSndCardRegister
    (
    VX_SND_SOC_CARD * pCard
    )
    {
    if (pCard->name == NULL || pCard->dev == NULL)
        {
        return ERROR;
        }

    DLL_INIT(&pCard->component_dev_list);
    DLL_INIT(&pCard->list);
    DLL_INIT(&pCard->rtd_list);

    pCard->instantiated = 0;
    pCard->cardMutex = semMCreate (SEM_Q_PRIORITY
                                   | SEM_DELETE_SAFE
                                   | SEM_INVERSION_SAFE);

    return snd_soc_bind_card(pCard);
    }

VX_SND_SOC_CARD * vxSndCardGet (void)
    {
    return gDebugCard;
    }

DL_LIST * vxSndCmpGet (void)
    {
    return &component_list;
    }
